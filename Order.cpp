/**
 * @file Order.cpp
 * @brief Order class implementation file
 * 
 * @authors Adam Valík (xvalik05), Marek Effenberger (xeffen00)
 * 
*/

#include "Order.h"

/**
 * @class ReferenceDeviceFailure
 * @brief Class representing the process of reference device failure
 * 
 * @details Used to simulate the time it takes to repair a reference device 
 * (usually sending it to the manufacturer)
 */
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

        Wait(Exponential(TIME_REFDEV_FAILURE_REPAIR)); // about a week

        if (isPrecise) {
            Leave(PreciseRefDev, 1);
        } else {
            Leave(UnpreciseRefDev, 1);
        }
    }
};

/**
 * @class SWDevelopment
 * @brief Class representing the process of software development
 * 
 * @details This class represents only the rest of the development time, after the order has arrived, 
 * waited, but the externist took his time and exceeded the timer, so this process had to be started 
 * to finish the SW development.
 */
class SWDevelopment : public Process {
    double workTime;
    double timer;

public:
    SWDevelopment(double workTime, double timer) : workTime(workTime), timer(timer) {}

    void Behavior() {
        Seize(Externist); // externist should not have a queue
        Wait(workTime - timer);
        if (isAuto < AUTO_TOP) isAuto += AUTO_INCREASE;
        Release(Externist);
    }
};

// ---------------------------------------------------------------------------------------------------

void Order::Behavior() {
    // rejecting orders
    if (!isPriority && (Random() < PROB_NOT_ACCREDITED || OrderQueue.Length() >= ORDER_QUEUE_SIZE)) {
        RejectedOrders++;
        return;
    }

    if (EXTERNIST) {
        if (!Externist.Busy()) {
            notifyExternist();
        } else {
            Wait(Exponential(TIME_ORDER_ARRIVAL));
        }
    } else {
        Wait(Exponential(TIME_ORDER_ARRIVAL));
    }
    
    // order is in the lab
    start = Time;


    while (!acquireWorkerOrManager()) {
        Passivate();
    }

    useReferenceDevice();
    performCalibration();
    
    if (secondLife) {
        // this happens when the refdev fails catastrophically, 
        // so the order has to be calibrated from the start
        while (!acquireWorkerOrManager()) {
            Passivate();
        }

        CatastrophicFailure = false;
        useReferenceDevice();
        performCalibration();
    }
        
    if (CatastrophicFailure) {
        return;
    }

    returnReferenceDevice();
    releaseResources();
    finalizeOrder(start);
    processNextOrderInQueue();
}

void Order::notifyExternist() {
    if (hasSW) { // check if the SW already exists
        Wait(Exponential(TIME_ORDER_ARRIVAL));
        return;
    } else { // hasSW = false && externist not busy
        Seize(Externist);
        double workTime = Normal(TIME_EXTERNIST_WORK, 6);
        double arrivalTime = Exponential(TIME_ORDER_ARRIVAL);
        double timer = (9 * 8); // set timer to 9 days

        if (workTime < timer && arrivalTime < timer) {
            // standard scenario, both made it in time
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
            // externist is released, SW waits for the order
            Wait(workTime);
            Release(Externist);
            hasSW = true;
            increaseAuto();
            Wait(arrivalTime - workTime);
            return;
        } else if (workTime > timer && arrivalTime < timer) {
            // SW development is taking too long, so the order is processed manually
            Wait(timer);
            hasSW = false;
            Release(Externist);
            (new SWDevelopment(workTime, timer))->Activate(); // rest of the SW development
            return;
        } else {
            // both exceeded the timer
            if (workTime < arrivalTime) {
                // externist is released, SW waits for the order
                Wait(workTime);
                Release(Externist);
                hasSW = true;
                increaseAuto();
                Wait(arrivalTime - workTime);
                return;
            } else {
                // SW development is taking too long, so the order is processed manually
                Wait(arrivalTime);
                hasSW = false;
                Release(Externist);
                (new SWDevelopment(workTime, timer))->Activate(); // rest of the SW development
                return;
            }
        }
    }
}

void Order::increaseAuto() {
    // SW goes to the portfolio, so the probability of the next order having SW increases
    if (isAuto < AUTO_TOP) isAuto += AUTO_INCREASE;
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
        // insert at the beginning
        OrderQueue.InsFirst(this);
    }
    else if (isPriority == 1) {
        // insert before the first non-priority order
        for (auto it = OrderQueue.begin(); it != OrderQueue.end(); ++it) {
            if (!dynamic_cast<Order*>(*it)->isPriority) {
                OrderQueue.PredIns(this, it);
                return;
            }
        }
        Into(OrderQueue); // or at the end
    } else {
        // non-priority goes at the end
        Into(OrderQueue);
    }
}

void Order::useReferenceDevice() {
    if (isPrecise) {
        Enter(PreciseRefDev, 1);
    } else {
        Enter(UnpreciseRefDev, 1);
    }
}

void Order::returnReferenceDevice() {
    if (isPrecise) {
        Leave(PreciseRefDev, 1);
    } else {
        Leave(UnpreciseRefDev, 1);
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
        handleCatastrophicError();
    }
}

void Order::handleCatastrophicError() {
    CatastrophicFailures++;
    CatastrophicFailure = true;
    double machineFailure = Random();

    if (machineFailure < PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH) {
        // reference device failure only (45%: 0-45)
        handleReferenceDeviceFailure();
    } else if (machineFailure < 2*(PROB_SINGLEDEV_BROKE - PROB_ERROR_BOTH)) {
        // order failure only (45%: 45-90)
        handleOrderFailure();
    } else {
        // both fail (10%: 90-100)
        BothFailuresCatastrophy++;
        handleBothFailure();
    }
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
    releaseResources();
    returnReferenceDevice();
    (new ReferenceDeviceFailure(isPrecise))->Activate();

    // order goes to the first position in the queue
    isWorkedOnByManager = false;
    isPriority = UBER_PRIORITY;
    secondLife = true;
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
            // activate the first order that can be processed (refdev is available)
            if ((nextOrder->isPrecise && !PreciseRefDev.Full()) ||
                (!nextOrder->isPrecise && !UnpreciseRefDev.Full())) {
                OrderQueue.Get(it)->Activate();
                break;
            }
        }
    }
}
