#include "simlib.h"
#include <iostream>

#define DAYS_SIMULATION 365
#define HOURS_SIMULATION (DAYS_SIMULATION * 24)

#define NUM_WORKERS 3

#define PROB_PRIORITY 0.2
#define PROB_PRECISE 0.2
#define PROB_NOT_ACCREDITED 0.1
#define PROB_ERROR 0.01
#define PROB_SMALL_ERROR 1 // TODO: katastrofy
#define PROB_ERROR_BOTH 0.05
#define PROB_RESULT_NOT_OK 0.5
#define PROB_REFDEV_BROKE 0.5

#define ORDER_QUEUE_SIZE 100

#define TIME_ORDER_GENERATION 8
#define TIME_ORDER_ARRIVAL 48
#define TIME_AUTO_CALIBRATION 8
#define TIME_MANUAL_CALIBRATION 14
#define TIME_REPAIR 1
#define TIME_TWEAK 1
#define TIME_WRITE_REPORT 1
#define TIME_RECALIBRATION_GENERATION (24 * 365 * 2) / ((numPreciseRefDev) + (numUnpreciseRefDev))
#define TIME_RECALIBRATION 24

double isAuto = 0.2;
int numPreciseRefDev = 2;
int numUnpreciseRefDev = 10;

Facility Manager("Manager");
Store Workers("Workers", NUM_WORKERS);
Queue OrderQueue("OrderQueue");
Queue ManagerQueue("ManagerQueue");

Store PreciseRefDev("PreciseRefDev", numPreciseRefDev);
Store UnpreciseRefDev("UnpreciseRefDev", numUnpreciseRefDev);

Stat ProcessingTime("Processing time");
Stat PriorityWaitTime("Priority wait time");

int RejectedOrders = 0;
int ProcessedOrders = 0;
int Errors = 0;
int Recalibrations = 0;

class Order : public Process {
    bool isPriority;
    bool isPrecise;
    bool hasSW;

public:
    Order(bool priority, bool precision) : isPriority(priority), isPrecise(precision) {}

    void Behavior() {
        if (!isPriority) {
            // only for not priority orders
            if (Random() < PROB_NOT_ACCREDITED || OrderQueue.Length() >= ORDER_QUEUE_SIZE) {
                // Order is not accredited or the queue is full
                RejectedOrders++;
                Passivate();
                return;
            }
        }



        // notification, wait for the order to arrive
        Wait(Exponential(TIME_ORDER_ARRIVAL));

        double start = Time;

        bool isManager;

        processing:
        // Attempt to acquire a worker, if none available attempt manager
        if (!Workers.Full()) {
            Enter(Workers, 1); // Acquire one of the workers
            isManager = false;
        } else if (!Manager.Busy()) {
            Seize(Manager, 0); // Try to acquire the manager, respecting priority
            isManager = true;
        } else {
            // If neither workers nor manager is available, put the order in the queue
            if (isPriority) {
                // Insert priority order just before the regular orders
                Queue::iterator it = OrderQueue.begin();
                for (; it != OrderQueue.end(); ++it) {
                    Order* orderInQueue = dynamic_cast<Order*>(*it);
                    if (!orderInQueue->isPriority) {
                        OrderQueue.PredIns(this, it); // Insert before the first non-priority order
                        break;
                    }
                }
                // If all orders are priority, add to the end
                if (it == OrderQueue.end()) {
                    Into(OrderQueue);
                }
            } else {
                // Insert regular order at the end of the queue
                Into(OrderQueue);
            }
            Passivate();
            goto processing;
        }

        // Availability of reference device is checked
        if (isPrecise) {
            Enter(PreciseRefDev, 1);
        } else {
            Enter(UnpreciseRefDev, 1);
        }

        // ma pracovnika i zarezini jupi, cas ho predurcit ke smrti
        auto timeOfCalibration = Random() < isAuto ? Exponential(TIME_AUTO_CALIBRATION) : Exponential(TIME_MANUAL_CALIBRATION);

        if (Random() < PROB_ERROR) {
            Errors++;
            auto totalDuration = Exponential(timeOfCalibration);
            double startTime = Time;
            Wait(Uniform(0, totalDuration));
            double errorTime = Time;
            double restOfDuration = totalDuration - (errorTime - startTime);

            int numErrors = Random() < PROB_ERROR_BOTH ? 2 : 1;

            if (Random() < PROB_SMALL_ERROR) {
                Wait(numErrors * Exponential(TIME_REPAIR) + restOfDuration);
            } else {
                // katastrofa    

            }
        } else {
            Wait(Exponential(timeOfCalibration));
        }

        if (Random() < PROB_RESULT_NOT_OK) {
            Wait(Exponential(TIME_TWEAK));
        }

        if (isPrecise) {
            Leave(PreciseRefDev, 1);
        } else {
            Leave(UnpreciseRefDev, 1);
        }

        if (isManager) {
            Release(Manager);
        } else {
            Leave(Workers, 1);
        }
    
        if (Manager.Busy()) {
            Into(ManagerQueue);
            Passivate();
        }
        Seize(Manager, 1);
        Wait(Exponential(TIME_WRITE_REPORT));
        Release(Manager);
        if (!ManagerQueue.Empty()) {
            ManagerQueue.GetFirst()->Activate();
        }

        // Record processing time
        ProcessingTime(Time - start);

        // Number of processed orders
        ProcessedOrders++;

        // If there are orders waiting in the queue, activate one of them
        if (!OrderQueue.Empty()) {
            Queue::iterator it = OrderQueue.begin();
            for (; it != OrderQueue.end(); ++it) {
                Order* orderInQueue = dynamic_cast<Order*>(*it);
                if (orderInQueue->isPrecise && !PreciseRefDev.Full()) {
                    break;
                } else if (!orderInQueue->isPrecise && !UnpreciseRefDev.Full()) {
                    break;
                }
            }
            if (it != OrderQueue.end()) {
                OrderQueue.Get(it)->Activate();
            }
        }
    }
};

class OrderGenerator : public Event {
    void Behavior() {
        (new Order(Random() < PROB_PRIORITY, Random() < PROB_PRECISE))->Activate();
        Activate(Time + Exponential(TIME_ORDER_GENERATION));
    }
};

class Recalibration : public Process {
    void Behavior() {
        double probPrecise = numPreciseRefDev / (numPreciseRefDev + numUnpreciseRefDev);
        bool isPrecise;
        if (Random() < probPrecise) {
            isPrecise = true;
            Enter(PreciseRefDev, 1);
        } else {
            isPrecise = false;
            Enter(UnpreciseRefDev, 1);
        }

        Wait(Exponential(TIME_RECALIBRATION));

        if (isPrecise) {
            Leave(PreciseRefDev, 1);
        } else {
            Leave(UnpreciseRefDev, 1);
        }

        Recalibrations++;
    }
};

class RecalibrationGenerator : public Event {
    void Behavior() {
        (new Recalibration())->Activate();
        Activate(Time + Exponential(TIME_RECALIBRATION_GENERATION));
    }
};

int main() {
    // Initialize the simulation (time from 0 to 24 hours)
    Init(0, HOURS_SIMULATION);

    // Activate the order generator
    (new OrderGenerator())->Activate();
    (new RecalibrationGenerator())->Activate();

    // Run the simulation
    Run();

    // Outputs
    Manager.Output();
    Workers.Output(); // Output for workers
    OrderQueue.Output(); // Output for the worker queue
    ProcessingTime.Output();
    PriorityWaitTime.Output();
    PreciseRefDev.Output();
    UnpreciseRefDev.Output();

    std::cout << "Number of processed orders: " << ProcessedOrders << std::endl;
    std::cout << "Number of rejected orders: " << RejectedOrders << std::endl;
    std::cout << "Number of errors: " << Errors << std::endl;
    std::cout << "Number of recalibrations: " << Recalibrations << std::endl;

    return 0;
}
