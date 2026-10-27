#ifndef CAN2CLUSTER_IO_H
#define CAN2CLUSTER_IO_H

#include "can2cluster_defs.h"

void basicInit(void);
void finishInit(void);
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
void broadcastPaddles(void *args);
void broadcastSpeed(void *args);
void parseGPS(void *args);
void parseDSG(void *args);
void needleSweep(void);
void startNeedleSweep(void);       // start a needle sweep running in its own task (does nothing if one is already running)
void needleSweepTask(void *args);  // the task that runs one sweep, then deletes itself

void setupBoardSwitches(void); // S3: set the CAN standby/termination and pull-up switch pins

// Light / reverse outputs (plain GPIO on both boards)
void driveReverse(bool on); // reverse light MOSFET on/off
void driveEPC(bool on);     // EPC light output on/off
void driveEML(bool on);     // EML light output on/off
void updateFuelPump(void);  // fuel pump relay on EML/EPC: prime at power-up, then on while CAN RPM is there

// Coolant temperature gauge (DevKit: PWM on the shared EML/EPC output, S3: MCP4725 current sink)
void applyCoolantOutput(void);              // DevKit: (re)attach or detach the LEDC channel to EML/EPC per coolantOutput
void updateCoolantOutput(void);             // compute the output from temp/cal and drive the gauge (call each output loop)
uint16_t coolantDutyForTemp(int16_t tempC); // interpolate the output value for a temperature from the calibration table

void queryECUTask(void *args);

#endif // CAN2CLUSTER_IO_H
