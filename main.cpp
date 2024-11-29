#include "simlib.h"
#include <iostream>
#include "Order.h"
#include "Constants.h"


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
