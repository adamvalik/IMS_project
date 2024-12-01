#include "Order.h"

class ReferenceDeviceFailure : public Process {
    bool isPrecise;

public:
    ReferenceDeviceFailure(bool precise) : isPrecise(precise) {}

    void Behavior() {

        if (isPrecise) {
            Enter(PreciseRefDev, 1);
        } else {
            Enter(UnpreciseRefDev, 1);
        }

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
        if (isAuto < 0.75) isAuto += AUTO_INCREASE;
        Release(Externist);
    }
};

void Order::notifyExternist() {
    if (hasSW) {
        waitForArrival();
        return;
    } else { // hasSW = false && externist not busy
        Seize(Externist);
        double workTime = Normal(4 * 8, 6);
        double arrivalTime = Exponential(2 * 8);
        double timer = (9 * 8);

        if (workTime < timer && arrivalTime < timer) {
            // normalni
            if (workTime < arrivalTime) {
                Wait(workTime);
                Release(Externist);
                hasSW = true;
                increaseAuto();
                Wait(arrivalTime - workTime);
                return;
            } else { // arrivalTime < workTime
                Wait(workTime);
                Release(Externist);
                hasSW = true;
                increaseAuto();
                return;
            }
        } else if (workTime < timer && arrivalTime > timer) {
            // pracovnik jde na dalsi, sw ceka na zakazku
            Wait(workTime);
            Release(Externist);
            hasSW = true;
            increaseAuto();
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
                increaseAuto();
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

void Order::increaseAuto() {
    if (isAuto < 0.75) isAuto += AUTO_INCREASE;
}

void Order::Behavior() {
    if (!isPriority && (Random() < PROB_NOT_ACCREDITED || OrderQueue.Length() >= ORDER_QUEUE_SIZE)) {
        RejectedOrders++;
        return;
    }

    if (!Externist.Busy()) {
        notifyExternist();
    } else {
        waitForArrival();
    }

    start = Time;

    secondLife:
    while (!acquireWorkerOrManager()) {
        Passivate();
    }

    useReferenceDevice();
    performCalibration();
    if (CatastrophicFailure) {
        return;
    }
    returnReferenceDevice();
    releaseResources();
    finalizeOrder(start);
    processNextOrderInQueue();
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
        Into(OrderQueue);
    } else {
        Into(OrderQueue);
    }
}

void Order::returnReferenceDevice() {
    if (isPrecise) {
        Leave(PreciseRefDev, 1);
    } else {
        Leave(UnpreciseRefDev, 1);
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
    if (KATASTROFAZNOVU) {
        printf("do satka, opetovna katastrofa");
    }
    CatastrophicFailure = true;
    double machineFailure = Random();

    handleReferenceDeviceFailure();


    // if (machineFailure < PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH) {
    //     // Reference device failure only (45%: 0-45)
    //     // handleReferenceDeviceFailure();
    // } else if (machineFailure < 2*(PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH)) {
    //     // Order failure only (45%: 45-90)
    //     handleOrderFailure();
    // } else {
    //     // Both fail (10%: 90-100)
    //     BothFailuresCatastrophy++;
    //     // handleBothFailure();
    // }
}

void Order::handleBothFailure() {
    releaseResources();
    returnReferenceDevice();
    (new ReferenceDeviceFailure(isPrecise))->Activate();
    processNextOrderInQueue();
} 

void Order::handleOrderFailure() {
    releaseResources();
    returnReferenceDevice();
    processNextOrderInQueue();
}

void Order::handleReferenceDeviceFailure() {
    // order goes to the first position in the queue
    releaseResources();
    returnReferenceDevice();
    (new ReferenceDeviceFailure(isPrecise))->Activate();

    // druhy zivot zakazky az do skoncovani (zadna treti sance)
    isWorkedOnByManager = false;
    CatastrophicFailure = false;
    KATASTROFAZNOVU = true;
    isPriority = UBER_PRIORITY;
    goto secondLife;
    // druhy zivot zakazky az do skoncovani (zadna treti sance)
}

void Order::releaseResources() {
    if (isWorkedOnByManager) {
        Release(Manager);
    } else {
        Leave(Workers, 1);
    }
}


void Order::finalizeOrder(double start) {
    while (Manager.Busy()) {
        Into(ManagerQueue);
        Passivate();
    }
    Seize(Manager, 1);
    Wait(Exponential(TIME_WRITE_REPORT));
    Release(Manager);
    
    if (!ManagerQueue.Empty()) {
        ManagerQueue.GetFirst()->Activate();
    }

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
