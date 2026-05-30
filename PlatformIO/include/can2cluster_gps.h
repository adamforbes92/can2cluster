#ifndef CAN2CLUSTER_GPS_H
#define CAN2CLUSTER_GPS_H

#include "can2cluster_defs.h"

unsigned long initGPS();
void parseGPS(void *args);
bool setGPSUpdateRate(uint8_t rateHz, String &responseMsg);
float getGPSUpdateFrequency();
int gpsAutoApplySecondsRemaining();
bool gpsAutoRateApplyPending();
static void printFloat(float val, bool valid, int len, int prec);

#endif // CAN2CLUSTER_GPS_H
