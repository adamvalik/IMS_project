/**
 * @file Order.h
 * @brief Order class header file
 * @date 2024-11-29
 * @project IMS project
 * @author Adam Valík - xvalik05, Marek Effenberger - xeffen00
 */

#ifndef IMS_PROJECT_ORDER_H
#define IMS_PROJECT_ORDER_H

#include <iostream>
#include <vector>
#include "simlib.h"
#include "Constants.h"

extern Facility Manager;
extern Store Workers;
extern Queue OrderQueue;
extern Store PreciseRefDev;
extern Store UnpreciseRefDev;

extern Stat ProcessingTime;
extern int RejectedOrders;
extern int ProcessedOrders;
extern int Errors;

class Order : public Process {

public:

    /**
     * @brief Construct a new Order object
     * @param priority
     * @param precision
     */
    Order(bool priority, bool precision) : isPriority(priority), isPrecise(precision) {}

    /**
     * @brief Destroy the Order object
     */
    ~Order() = default;

    /**
     * @brief Order behavior
     */
    void Behavior();

private:

    /**
     * @brief Bool representing whether the order shall be treated as priority
     */
    bool isPriority;

    /**
     * @brief Bool representing whether the order shall be treated as in need of a precise reference device
     */
    bool isPrecise;

    /**
     * @brief Bool representing whether the order shall be treated as having a software SCPI control
     */
    bool hasSW;

    bool isWorkedOnByManager = false;

    void handleRejectedOrder();
    void waitForArrival();
    bool acquireWorkerOrManager(bool& isManager);
    void useReferenceDevice();
    void performCalibration();
    void handleCalibrationError(double timeOfCalibration);
    void releaseResources(bool isManager);
    void finalizeOrder(double start);
    void processNextOrderInQueue();
    void handleCatastrophicError();

};


#endif //IMS_PROJECT_ORDER_H
