#ifndef CAN2CLUSTER_GPS_H
#define CAN2CLUSTER_GPS_H

#include "can2cluster_defs.h"

void parseGPS(void *args);
static void printFloat(float val, bool valid, int len, int prec);

#endif // CAN2CLUSTER_GPS_H
