#ifndef CAN2CLUSTER_CAN_H
#define CAN2CLUSTER_CAN_H

#include "can2cluster_defs.h"

void canInit(void);
void canReceiveTask(void *args);
void canMonitorTask(void *args);
void onBodyRX(const twai_message_t& frame);
void sendPaddleUpFrame(void);
void sendPaddleDownFrame(void);
void broadcastGRA(void *args);
void broadcastSpeed(void *args);

#endif // CAN2CLUSTER_CAN_H
