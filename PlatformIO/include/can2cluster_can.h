#ifndef CAN2CLUSTER_CAN_H
#define CAN2CLUSTER_CAN_H

#include "can2cluster_defs.h"

void canInit(void);
void canReceiveTask(void *args);
void canMonitorTask(void *args);
void onBodyRX(const twai_message_t& frame);
void sendMqbPaddleFrame(uint8_t paddleBits); // one MQB Gateway_74 (0x3DD) frame
void sendGraFrame(uint8_t cruiseButtons, uint8_t setResumeBits, uint8_t paddleBits); // one PQ GRA_Neu (0x38A) frame
void broadcastPaddles(void *args);
void broadcastSpeed(void *args);

#endif // CAN2CLUSTER_CAN_H
