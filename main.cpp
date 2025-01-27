/**
 * @file main.cpp
 * @brief Main file for the simulation
 * 
 * @authors Adam Valík (xvalik05), Marek Effenberger (xeffen00)
 * 
*/

#include "simlib.h"
#include <iostream>
#include "Order.h"
#include "Constants.h"
#include <ctime>

double isAuto = 0.2;
int numPreciseRefDev = 2;
int numUnpreciseRefDev = 10;

Facility Manager("Manager");
Facility Externist("Externist");
Store Workers("Workers", NUM_WORKERS);
Queue OrderQueue("OrderQueue");
Queue ManagerQueue("ManagerQueue");

Store PreciseRefDev("PreciseRefDev", numPreciseRefDev);
Store UnpreciseRefDev("UnpreciseRefDev", numUnpreciseRefDev);

Stat ProcessingTime("Processing time");
Stat PriorityWaitTime("Priority wait time");

// statistics
int RejectedOrders = 0;
int ProcessedOrders = 0;
int Errors = 0;
int Recalibrations = 0;
int CatastrophicFailures = 0;
int BothFailuresCatastrophy = 0;

class OrderGenerator : public Event {
    void Behavior() {
        (new Order(Random() < PROB_PRIORITY ? 1 : 0, Random() < PROB_PRECISE, Random() < isAuto))->Activate();
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
    Init(0, HOURS_SIMULATION);

    RandomSeed(time(NULL));

    (new OrderGenerator())->Activate();
    (new RecalibrationGenerator())->Activate();

    Run();

    Externist.Output();
    Manager.Output();
    Workers.Output();
    OrderQueue.Output();
    ProcessingTime.Output();
    PriorityWaitTime.Output();
    PreciseRefDev.Output();
    UnpreciseRefDev.Output();
    ManagerQueue.Output();

    std::cout << "Number of processed orders: " << ProcessedOrders << std::endl;
    std::cout << "Number of rejected orders: " << RejectedOrders << std::endl;
    std::cout << "Number of errors: " << Errors << std::endl;
    std::cout << "Number of recalibrations: " << Recalibrations << std::endl;
    std::cout << "Number of catastrophic failures: " << CatastrophicFailures << std::endl;
    std::cout << "Number of both failures catastrophies: " << BothFailuresCatastrophy << std::endl;

    return 0;
}
