#ifndef CAN2CLUSTER_IO_H
#define CAN2CLUSTER_IO_H

#include "can2cluster_defs.h"

void basicInit(void);
void setupPins(void);
void setupButtons(void);
void setupTasks(void);
void showState(void *arg);
void updateSpeed(void *args);
void updateRPM(void *args);
void outputControlTask(void *args);
void paddleFeedbackTask(void *args);
void checkError(void *args);
void writeEEP(void *args);
void broadcastGRA(void *args);
void broadcastSpeed(void *args);
void parseGPS(void *args);
void parseDSG(void *args);
void gpsResumeTask(void *args);
void needleSweep(void);

void queryECUTask(void *args);

#endif // CAN2CLUSTER_IO_H
