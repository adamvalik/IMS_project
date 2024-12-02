/**
 * @file Order.h
 * @brief Order class header file
 * 
 * @authors Adam Valík (xvalik05), Marek Effenberger (xeffen00)
 * 
*/

#ifndef IMS_PROJECT_ORDER_H
#define IMS_PROJECT_ORDER_H

#include <iostream>
#include <vector>
#include "simlib.h"
#include "Constants.h"

extern Facility Externist;
extern Facility Manager;
extern Queue ManagerQueue;

extern Store Workers;
extern Store PreciseRefDev;
extern Store UnpreciseRefDev;
extern Queue OrderQueue;

extern Stat PriorityWaitTime;
extern Stat ProcessingTime;
extern int RejectedOrders;
extern int ProcessedOrders;
extern int Errors;
extern int CatastrophicFailures;
extern int BothFailuresCatastrophy;

/**
 * @class Order
 * Order class representing the process of an order in the lab
 */
class Order : public Process {

public:

    /**
     * @brief Construct a new Order object
     * @param priority Integer representing the order's priority
     * @param precision Boolean indicating if the order requires a precise reference device
     * @param isAuto Boolean indicating if the order has software SCPI control
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
     * @brief Integer representing the order's priority
     */
    int isPriority;

    /**
     * @brief Boolean indicating if the order requires a precise reference device
     */
    bool isPrecise;

    /**
     * @brief Boolean indicating if the order has software SCPI control
     */
    bool hasSW;

    /**
     * @brief Start time of the order
     */
    double start = 0.0;

    /**
     * @brief Bool representing whether the order is being worked on by a manager
     */
    bool isWorkedOnByManager = false;

    /**
     * @brief Bool representing whether the order has a catastrophic failure
     */
    bool CatastrophicFailure = false;

    /**
     * @brief Bool representing whether the order has a second life
     */
    bool secondLife = false;

    /**
     * @brief Acquire a worker or manager for the order
     * @return True if a worker or manager is acquired, false otherwise
     */
    bool acquireWorkerOrManager();

    /**
     * @brief Use the reference device for the order
     */
    void useReferenceDevice();

    /**
     * @brief Perform calibration for the order
     */
    void performCalibration();

    /**
     * @brief Handle calibration error
     * @param timeOfCalibration The time at which the calibration error occurred
     */
    void handleCalibrationError(double timeOfCalibration);

    /**
     * @brief Release resources used by the order
     */
    void releaseResources();

    /**
     * @brief Finalize the order
     * @param start The start time of the order
     */
    void finalizeOrder(double start);

    /**
     * @brief Process the next order in the queue
     */
    void processNextOrderInQueue();

    /**
     * @brief Handle catastrophic error for the order
     */
    void handleCatastrophicError();

    /**
     * @brief Handle reference device failure for the order
     */
    void handleReferenceDeviceFailure();

    /**
     * @brief Handle order failure
     */
    void handleOrderFailure();

    /**
     * @brief Handle both order and reference device failure
     */
    void handleBothFailure();

    /**
     * @brief Put the order into the queue
     */
    void intoQueue();

    /**
     * @brief Return the reference device used by the order
     */
    void returnReferenceDevice();

    /**
     * @brief Increase the auto count for the order
     */
    void increaseAuto();

    /**
     * @brief Notify the externist about the order
     */
    void notifyExternist();
};

#endif //IMS_PROJECT_ORDER_H