/**
 * @file Constants.h
 * @brief Constants for the simulation
 * 
 * @authors Adam Valík (xvalik05), Marek Effenberger (xeffen00)
 * 
*/

#ifndef IMS_PROJECT_CONSTANTS_H
#define IMS_PROJECT_CONSTANTS_H

// in the simulation, the day is 8 hours long (meaning working hours)
#define YEARS_SIMULATION 30
#define HOURS_SIMULATION (YEARS_SIMULATION * 365 * 8)

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

#define EXTERNIST true // switch for the modes w/ | w/o externist

#define TIME_ORDER_GENERATION 4
#define TIME_ORDER_ARRIVAL (2 * 8)
#define TIME_AUTO_CALIBRATION 8
#define TIME_MANUAL_CALIBRATION 14
#define TIME_REPAIR 1
#define TIME_TWEAK 1
#define TIME_WRITE_REPORT 1
#define TIME_RECALIBRATION_GENERATION (2 * 8 * 365) / ((numPreciseRefDev) + (numUnpreciseRefDev))
#define TIME_RECALIBRATION (4 * 8)
#define TIME_REFDEV_FAILURE_REPAIR (7 * 8)
#define TIME_EXTERNIST_WORK (4 * 8)

// probability that the lab has automatic calibration software for the incoming order
extern double isAuto;

extern int numPreciseRefDev;
extern int numUnpreciseRefDev;

#endif //IMS_PROJECT_CONSTANTS_H
