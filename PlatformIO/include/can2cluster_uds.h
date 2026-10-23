#ifndef CAN2CLUSTER_UDS_H
#define CAN2CLUSTER_UDS_H

#include "can2cluster_defs.h"
#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// ---------------------------------------------------------------------------
// VW TP2.0 (KWP2000 over TP2.0) — DSG module 0x02, MK5/PQ35 platform
// CAN IDs confirmed from VCDS SavvyCAN capture:
//   Broadcast (channel setup) : 0x200  (ESP32 → ECU)
//   DSG setup response         : 0x202  (ECU → ESP32)
//   Tester data channel TX     : 0x760  (ESP32 → DSG, negotiated)
//   DSG data channel RX        : 0x300  (DSG → ESP32, negotiated)
// ---------------------------------------------------------------------------
#define TP20_BROADCAST_ID   0x200U  // Channel-setup broadcast
#define TP20_DSG_MODULE     0x02U   // DSG logical module address
#define TP20_DSG_SETUP_RX   0x202U  // Setup response from DSG (0x200 + module)
#define TP20_TX_ID          0x760U  // ESP32 → DSG data channel (negotiated)
#define TP20_RX_ID          0x300U  // DSG → ESP32 data channel (negotiated)

// KWP2000 services used over TP2.0
#define KWP_START_DIAG_SESSION  0x10U
#define KWP_RDBLI               0x21U  // ReadDataByLocalIdentifier
#define KWP_RDBLI_POS           0x61U  // Positive response
#define KWP_DIAG_MODE_DEV       0x89U  // Development/coding session
#define KWP_MEAS_GROUP          0x01U  // Measuring block group 01

// Speed formula: raw 16-bit (big-endian) after formula-ID byte 0x25
// speed_km_h = raw / 3.0  (confirmed: raw=58 @ ~19 km/h)
#define TP20_SPEED_FORMULA_ID   0x25U
#define TP20_SPEED_SCALE_DIV    3.0f

// ---------------------------------------------------------------------------
// UDS (ISO-TP / ISO 14229) speed polling
// Default IDs and DID — adjust per vehicle/ECU via defines below
// ---------------------------------------------------------------------------
// MQB DSG (DQ250-MQB / DQ381) answers UDS on its own physical IDs, not the
// OBD2 broadcast. DID 0x100A = "Vehicle speed from transmission" (the gearbox's
// own theoretical road speed, the MQB equivalent of the PQ TP2.0 raw/3 value).
// From the firmware RE + opendbc/OBDb: 16-bit, LITTLE-endian, signed,
// km/h = raw/10. The firmware carries road speed at 0.1 km/h per LSB (confirmed:
// a divide-by-10 against a km/h limit). See Documents/dsg_re/DSG_RE_SUMMARY.md.
// ** CONFIRM ON THE CAR ONCE **: read 22 10 0A on 0x7E1/0x7E9 at a known speed;
// the DID table itself is in a part of the flash we couldn't dump, so the DID
// number and the raw/10 are from the RE + community data, not byte-proven.
#define UDS_TX_ID           0x7E1U  // DSG physical request (DQ250-MQB / DQ381)
#define UDS_RX_ID           0x7E9U  // DSG physical response
#define UDS_SPEED_DID       0x100AU // "Vehicle speed from transmission": 16-bit LE signed, km/h = raw/10

// ISO-TP PCI nibbles
#define ISOTP_SF  0x00U  // Single Frame
#define ISOTP_FF  0x10U  // First Frame
#define ISOTP_CF  0x20U  // Consecutive Frame
#define ISOTP_FC  0x30U  // Flow Control

// UDS Service IDs
#define UDS_SID_SESSION_CTRL    0x10U
#define UDS_SID_TESTER_PRESENT  0x3EU
#define UDS_SID_READ_DID        0x22U
#define UDS_SID_READ_DID_POS    0x62U  // Positive response

// FreeRTOS queues shared between CAN RX task and protocol tasks
extern QueueHandle_t tp20RxQueue;  // 0x202 setup + 0x300 data frames → TP2.0 task
extern QueueHandle_t udsRxQueue;   // UDS_RX_ID frames → UDS task

// FreeRTOS task entry points (created in can2cluster_tasks.cpp)
void taskTP20(void *arg);
void taskUDS(void *arg);

#endif // CAN2CLUSTER_UDS_H
