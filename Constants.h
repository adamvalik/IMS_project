//
// Created by marek on 29.11.2024.
//

#ifndef IMS_PROJECT_CONSTANTS_H
#define IMS_PROJECT_CONSTANTS_H

#define DAYS_SIMULATION 365
#define HOURS_SIMULATION (DAYS_SIMULATION * 24)

#define NUM_WORKERS 3

#define PROB_PRIORITY 0.2
#define PROB_PRECISE 0.2
#define PROB_NOT_ACCREDITED 0.1
#define PROB_ERROR 0.01
#define PROB_SMALL_ERROR 1
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

extern double isAuto;
extern int numPreciseRefDev;
extern int numUnpreciseRefDev;

#endif //IMS_PROJECT_CONSTANTS_H
