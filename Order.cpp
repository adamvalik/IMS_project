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

void Order::Behavior() {
    if (!isPriority && (Random() < PROB_NOT_ACCREDITED || OrderQueue.Length() >= ORDER_QUEUE_SIZE)) {
        handleRejectedOrder();
        return;
    }

    waitForArrival();

    double start = Time;
    bool isManager = false;

    while (!acquireWorkerOrManager(isManager)) {
        Passivate();
    }

    useReferenceDevice();
    performCalibration();
    if (CatastrophicFailure){
        return;
    }
    releaseResources(isManager);
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


bool Order::acquireWorkerOrManager(bool& isManager) {
    if (!Workers.Full()) {
        Enter(Workers, 1);
        isManager = false;
        return true;
    } else if (!Manager.Busy()) {
        Seize(Manager, 0);
        isManager = true;
        isWorkedOnByManager = true;
        return true;
    } else {
        if (isPriority) {
            // Insert before the first non-priority order
            for (auto it = OrderQueue.begin(); it != OrderQueue.end(); ++it) {
                if (!dynamic_cast<Order*>(*it)->isPriority) {
                    OrderQueue.PredIns(this, it);
                    return false;
                }
            }
        }
        Into(OrderQueue); // Add at the end for non-priority
        return false;
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
    CatastrophicFailure = true;
    double machineFailure = Random();

    if (machineFailure < PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH) {
        // Reference device failure only (45%: 0-45)
        handleReferenceDeviceFailure();
    } else if (machineFailure < 2*(PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH)) {
        // Order failure only (45%: 45-90)
        handleOrderFailure();
    } else {
        // Both fail (10%: 90-100) TODO: THIS MIGHT NOT BE CORRECT AS THEY WORK WITH SAME RESOURCE
        handleReferenceDeviceFailure();
        handleOrderFailure();
    }
}


void Order::handleOrderFailure() {

    if (isPrecise) {
        Leave(PreciseRefDev, 1);
    } else {
        Leave(UnpreciseRefDev, 1);
    }

    releaseResources(isWorkedOnByManager);

    // Remove the order permanently from the system
    Passivate();
}

void Order::handleReferenceDeviceFailure() {
    // order goes to the first position in the queue
    (new ReferenceDeviceFailure(isPrecise))->Activate();
    releaseResources(isWorkedOnByManager);
    OrderQueue.InsFirst(this);
    Passivate();
    processNextOrderInQueue();
}

void Order::releaseResources(bool isManager) {
    if (isManager) {
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
