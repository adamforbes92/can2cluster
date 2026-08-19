#ifndef CAN2CLUSTER_IO_H
#define CAN2CLUSTER_IO_H

#include "can2cluster_defs.h"

void basicInit(void);
void setupPins(void);
void setupButtons(void);
void setupTasks(void);
void tasksSuspendAll(void);
void tasksResumeAll(void);
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
void needleSweep(void);

// Coolant temperature gauge (PWM on the shared EML/EPC ULN2003 output)
void applyCoolantOutput(void);              // (re)attach or detach the LEDC channel to EML/EPC per coolantOutput
void updateCoolantOutput(void);             // compute duty from temp/cal and drive the gauge (call each output loop)
uint16_t coolantDutyForTemp(int16_t tempC); // interpolate duty (0-1023) for a temperature from the calibration table

void queryECUTask(void *args);

#endif // CAN2CLUSTER_IO_H
