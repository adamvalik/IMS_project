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

extern Facility Externist;

extern Facility Manager;
extern Store Workers;
extern Queue OrderQueue;
extern Queue ManagerQueue;
extern Store PreciseRefDev;
extern Store UnpreciseRefDev;

extern Stat ProcessingTime;
extern int RejectedOrders;
extern int ProcessedOrders;
extern int Errors;
extern int CatastrophicFailures;
extern int BothFailuresCatastrophy;

class Order : public Process {

public:

    /**
     * @brief Construct a new Order object
     * @param priority
     * @param precision
     */
    Order(int priority, bool precision, bool isAuto) : isPriority(priority), isPrecise(precision), hasSW(isAuto) {}

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
    int isPriority;

    /**
     * @brief Bool representing whether the order shall be treated as in need of a precise reference device
     */
    bool isPrecise;

    /**
     * @brief Bool representing whether the order shall be treated as having a software SCPI control
     */
    bool hasSW;

    double start = 0.0;
    bool isWorkedOnByManager = false;

    bool CatastrophicFailure = false;
    bool secondLife = false;

    void waitForArrival();
    bool acquireWorkerOrManager();
    void useReferenceDevice();
    void performCalibration();
    void handleCalibrationError(double timeOfCalibration);
    void releaseResources();
    void finalizeOrder(double start);
    void processNextOrderInQueue();
    void handleCatastrophicError();
    void handleReferenceDeviceFailure();
    void handleOrderFailure();
    void handleBothFailure();
    void intoQueue();
    void returnReferenceDevice();
    void increaseAuto();

    void notifyExternist();


};


#endif //IMS_PROJECT_ORDER_H
