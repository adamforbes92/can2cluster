#ifndef CAN2CLUSTER_CRUISE_H
#define CAN2CLUSTER_CRUISE_H

#include "can2cluster_defs.h"

// ----------------------------------------------------------------------------
// MK4 cruise stalk (V4.02).
//
// With "Input Mode" set to MK4 Cruise Stalk, connector pins 10, 11 and 12 are
// the cruise stalk instead of the paddles and the hall speed input:
//   pin 10 = main switch, pin 11 = SET, pin 12 = RESUME (each on when grounded)
// broadcastPaddles() (can2cluster_can.cpp) calls updateCruiseStalk() every 10 ms
// and sends the result to the ECU in GRA_Neu (0x38A) every 20 ms.
// ----------------------------------------------------------------------------

void updateCruiseStalk(bool isMainOnNow, bool isSetPressedNow, uint32_t now); // read the stalk, set cruiseButtonsByte / cruiseSetResumeBits

#endif // CAN2CLUSTER_CRUISE_H
