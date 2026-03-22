#ifndef CAN2CLUSTER_DSG_H
#define CAN2CLUSTER_DSG_H

#include "can2cluster_defs.h"

double dq250_gear_ratio(uint8_t gear);
double dq250_final(uint8_t gear);
double dq250_speed(uint16_t rpm_in, uint8_t gear);
void parseDSG(void *args);

#endif // CAN2CLUSTER_DSG_H
