#ifndef CAN2CLUSTER_CAN_H
#define CAN2CLUSTER_CAN_H

#include "can2cluster_defs.h"

void canInit(void);
void canReceiveTask(void *args);
void onBodyRX(const twai_message_t& frame);
void sendPaddleUpFrame(void);
void sendPaddleDownFrame(void);
void broadcastGRA(void *args);
void broadcastSpeed(void *args);

// VW diag/allowlist query helpers
void queryDSG_TP20(void);
void queryHaldex_UDS(void);
void queryECUTask(void *args);

#endif // CAN2CLUSTER_CAN_H
