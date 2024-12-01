#include "Order.h"

class ReferenceDeviceFailure : public Process {
    bool isPrecise;

public:
    ReferenceDeviceFailure(bool precise) : isPrecise(precise) {}

    void Behavior() {
        Wait(Exponential(TIME_REFDEV_FAILURE_REPAIR));

        if (isPrecise) {
            Leave(PreciseRefDev, 1);
        } else {
            Leave(UnpreciseRefDev, 1);
        }
    }
};

class Dodelavka : public Process {
    double workTime;
    double timer;

public:
    Dodelavka(double workTime, double timer) : workTime(workTime), timer(timer) {}


    void Behavior() {
        Seize(Externist); // check ze na externistu neexistuje fronta
        Wait(workTime - timer);
        if (isAuto < 0.75) isAuto += 0.005;
        Release(Externist);
    }
};

void Order::notifyExternist() {
    if (hasSW) {
        waitForArrival();
        return;
    } else { // hasSW = false && externist not busy
        Seize(Externist);
        double workTime = Exponential(3 * 24);
        double arrivalTime = Exponential(2 * 24);
        double timer = Exponential(9 * 24);

        if (workTime < timer && arrivalTime < timer) {
            // normalni
            if (workTime < arrivalTime) {
                Wait(workTime);
                Release(Externist);
                hasSW = true;
                if (isAuto < 0.75) isAuto += 0.005;
                Wait(arrivalTime - workTime);
                return;
            } else { // arrivalTime < workTime
                Wait(workTime);
                Release(Externist);
                hasSW = true;
                if (isAuto < 0.75) isAuto += 0.005;
                return;
            }
        } else if (workTime < timer && arrivalTime > timer) {
            // pracovnik jde na dalsi, sw ceka na zakazku
            Wait(workTime);
            Release(Externist);
            hasSW = true;
            if (isAuto < 0.75) isAuto += 0.005;
            Wait(arrivalTime - workTime);
            return;
        } else if (workTime > timer && arrivalTime < timer) {
            // zakazka se jde zpracovat manulane, sw se hodi do portfolia jak se dodela
            Wait(timer);
            hasSW = false;
            Release(Externist);
            (new Dodelavka(workTime, timer))->Activate();
            return;
        } else {
            // obe byly pozdeji nez timer
            if (workTime < arrivalTime) {
                // pracovnik jde na dalsi, sw ceka na zakazku
                Wait(workTime);
                Release(Externist);
                hasSW = true;
                if (isAuto < 0.75) isAuto += 0.005;
                Wait(arrivalTime - workTime);
                return;
            } else {
                // zakazka se jde zpracovat manulane, sw se hodi do portfolia jak se dodela
                Wait(arrivalTime);
                hasSW = false;
                Release(Externist);
                (new Dodelavka(workTime, timer))->Activate();
                return;
            }
        }
    }
}

void Order::Behavior() {
    if (!isPriority && (Random() < PROB_NOT_ACCREDITED || OrderQueue.Length() >= ORDER_QUEUE_SIZE)) {
        handleRejectedOrder();
        return;
    }

    if (!Externist.Busy()) {
        notifyExternist();
    } else {
        waitForArrival();
    }

    start = Time;

    while (!acquireWorkerOrManager()) {
        Passivate();
    }

    useReferenceDevice();
    performCalibration();
    if (CatastrophicFailure) {
        return;
    }
    releaseResources();
    finalizeOrder(start);
    processNextOrderInQueue();
}


void Order::handleRejectedOrder() {
    RejectedOrders++;
    Passivate();
}


void Order::waitForArrival() {
    Wait(Exponential(TIME_ORDER_ARRIVAL));
}


bool Order::acquireWorkerOrManager() {
    if (!Workers.Full()) {
        Enter(Workers, 1);
        isWorkedOnByManager = false;
        return true;
    } else if (!Manager.Busy()) {
        Seize(Manager, 0);
        isWorkedOnByManager = true;
        return true;
    } else {
        intoQueue();
        return false;
    }
}

void Order::intoQueue() {
    if (isPriority == UBER_PRIORITY) {
        OrderQueue.InsFirst(this);
    }
    else if (isPriority == 1) {
        // Insert before the first non-priority order
        for (auto it = OrderQueue.begin(); it != OrderQueue.end(); ++it) {
            if (!dynamic_cast<Order*>(*it)->isPriority) {
                OrderQueue.PredIns(this, it);
                return;
            }
        }
    } else {
        Into(OrderQueue); // Add at the end for non-priority
    }
}


void Order::useReferenceDevice() {
    if (isPrecise) {
        Enter(PreciseRefDev, 1);
    } else {
        Enter(UnpreciseRefDev, 1);
    }
}


void Order::performCalibration() {
    auto timeOfCalibration = hasSW ? Exponential(TIME_AUTO_CALIBRATION) : Exponential(TIME_MANUAL_CALIBRATION);

    if (Random() < PROB_ERROR) {
        handleCalibrationError(timeOfCalibration);
    } else {
        Wait(Exponential(timeOfCalibration));
    }

    if (CatastrophicFailure){
        return;
    }

    if (Random() < PROB_RESULT_NOT_OK) {
        Wait(Exponential(TIME_TWEAK));
    }

    if (isPrecise) {
        Leave(PreciseRefDev, 1);
    } else {
        Leave(UnpreciseRefDev, 1);
    }
}


void Order::handleCalibrationError(double timeOfCalibration) {
    Errors++;
    auto totalDuration = Exponential(timeOfCalibration);
    double startTime = Time;
    Wait(Uniform(0, totalDuration));
    double errorTime = Time;
    double restOfDuration = totalDuration - (errorTime - startTime);

    int numErrors = Random() < PROB_ERROR_BOTH ? 2 : 1;

    if (Random() < PROB_SMALL_ERROR) {
        if (numErrors == 2) {
            Wait(Exponential(TIME_REPAIR) + Exponential(TIME_REPAIR) + restOfDuration);
        } else {
            Wait(Exponential(TIME_REPAIR) + restOfDuration);
        }
    } else {
        // Handle catastrophic errors if necessary
        handleCatastrophicError();
    }
}


void Order::handleCatastrophicError() {
    // Handle catastrophic errors if necessary
    CatastrophicFailures++;
    if (CatastrophicFailure) {
        printf("do satka, opetovna katastrofa");
    }
    CatastrophicFailure = true;
    double machineFailure = Random();

    if (machineFailure < PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH) {
        // Reference device failure only (45%: 0-45)
        handleReferenceDeviceFailure();
    } else if (machineFailure < 2*(PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH)) {
        // Order failure only (45%: 45-90)
        handleOrderFailure();
    } else {
        // Both fail (10%: 90-100)
        BothFailuresCatastrophy++;
        handleBothFailure();
    }
}

void Order::handleBothFailure() {
    (new ReferenceDeviceFailure(isPrecise))->Activate();
    releaseResources();
    processNextOrderInQueue();
} 

void Order::handleOrderFailure() {

    if (isPrecise) {
        Leave(PreciseRefDev, 1);
    } else {
        Leave(UnpreciseRefDev, 1);
    }

    releaseResources();
    processNextOrderInQueue();
}

void Order::handleReferenceDeviceFailure() {
    // order goes to the first position in the queue
    (new ReferenceDeviceFailure(isPrecise))->Activate();
    releaseResources();
    
    // druhy zivot zakazky az do skoncovani (zadna treti sance)
    isPriority = UBER_PRIORITY;
    intoQueue();
    Passivate();

    useReferenceDevice();
    performCalibration();
    releaseResources();
    finalizeOrder(start);
    processNextOrderInQueue();
}

void Order::releaseResources() {
    if (isWorkedOnByManager) {
        Release(Manager);
    } else {
        Leave(Workers, 1);
    }
}


void Order::finalizeOrder(double start) {
    if (!Manager.Busy()) {
        Seize(Manager, 1);
        Wait(Exponential(TIME_WRITE_REPORT));
        Release(Manager);
    } else {
        // If the manager is busy, move the order to the ManagerQueue
        Into(ManagerQueue);
        Passivate();
    }

    // Activate the next order waiting in the ManagerQueue, if any
    if (!ManagerQueue.Empty()) {
        ManagerQueue.GetFirst()->Activate();
    }

    // Record processing time and update statistics
    ProcessingTime(Time - start);
    ProcessedOrders++;
}

void Order::processNextOrderInQueue() {
    if (!OrderQueue.Empty()) {
        for (auto it = OrderQueue.begin(); it != OrderQueue.end(); ++it) {
            Order* nextOrder = dynamic_cast<Order*>(*it);
            if ((nextOrder->isPrecise && !PreciseRefDev.Full()) ||
                (!nextOrder->isPrecise && !UnpreciseRefDev.Full())) {
                OrderQueue.Get(it)->Activate();
                break;
            }
        }
    }
}
