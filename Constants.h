//
// Created by marek on 29.11.2024.
//

#ifndef IMS_PROJECT_CONSTANTS_H
#define IMS_PROJECT_CONSTANTS_H

#define DAYS_SIMULATION 30*365
#define HOURS_SIMULATION (DAYS_SIMULATION * 8)

#define NUM_WORKERS 3

#define PROB_PRIORITY 0.2
#define PROB_PRECISE 0.2
#define PROB_NOT_ACCREDITED 0.1
#define PROB_ERROR 0.05
#define PROB_SMALL_ERROR 0.90
#define PROB_ERROR_BOTH 0.05
#define PROB_RESULT_NOT_OK 0.5
#define PROB_SINGLEDEV_BROKE 0.5

#define ORDER_QUEUE_SIZE 20

#define UBER_PRIORITY 2

#define AUTO_INCREASE 0.002
#define AUTO_TOP 0.75

#define TIME_ORDER_GENERATION 4
#define TIME_ORDER_ARRIVAL 16
#define TIME_AUTO_CALIBRATION 8
#define TIME_MANUAL_CALIBRATION 14
#define TIME_REPAIR 1
#define TIME_TWEAK 1
#define TIME_WRITE_REPORT 1
#define TIME_RECALIBRATION_GENERATION (8 * 365 * 2) / ((numPreciseRefDev) + (numUnpreciseRefDev))
#define TIME_RECALIBRATION (8 * 4)
#define TIME_REFDEV_FAILURE_REPAIR (8 * 7)

extern double isAuto;
extern int numPreciseRefDev;
extern int numUnpreciseRefDev;

#endif //IMS_PROJECT_CONSTANTS_H
