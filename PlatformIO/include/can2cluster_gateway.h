#ifndef CAN2CLUSTER_GATEWAY_H
#define CAN2CLUSTER_GATEWAY_H

#include "can2cluster_defs.h"

// ----------------------------------------------------------------------------
// PQ gateway frames (V4.02).
//
// For a PQ35/46 engine and DSG in a car without the right gateway: when
// "Send PQ Gateway Frames" is ticked, Can2Cluster sends the frames a gateway
// would, so the ECU and DSG stop logging them as missing:
//   0x570 BSG_Last    every 100 ms - ignition on / engine running + battery voltage
//   0x390 Gate_Komf_1 every 100 ms - reverse gear
//   0x7D0 Diagnose_1  every 500 ms - mileage, copied from the cluster's Kombi_3 (0x520)
// Each one goes quiet for a second whenever another module sends that ID.
// ----------------------------------------------------------------------------

void gatewayFramesTask(void *args);                 // sends the frames (task, started in setupTasks())
void onGatewayFrameRX(const twai_message_t &frame); // called by onBodyRX() for 0x570 / 0x390 / 0x7D0 / 0x520
uint8_t gatewayFrameState(uint32_t otherSenderMs);  // GATEWAY_STATE_x for the web page
bool isClusterKmFresh(void);                        // true when the cluster sent its mileage in the last 5 s

#endif // CAN2CLUSTER_GATEWAY_H
