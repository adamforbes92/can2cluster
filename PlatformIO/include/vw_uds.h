#ifndef VW_UDS_H
#define VW_UDS_H

#include "can2cluster_defs.h"
#include <driver/twai.h>

// =====================================================================
// VW UDS / TP2.0 support is currently DISABLED.
// The previous implementation was sending requests to broadcast IDs
// (0x540 mWaehlhebel_1_ID, 0x2C0 HALDEX_ID) which collide with real
// ECU broadcasts on the powertrain bus and drive the TWAI controller
// into bus-off. Set VW_UDS_ENABLED to 1 to re-enable once the
// addressing / TP2.0 channel setup has been reworked.
// =====================================================================
#ifndef VW_UDS_ENABLED
#define VW_UDS_ENABLED 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Query helpers for VW ECUs
bool vw_uds_send_request(uint32_t id, const uint8_t* payload, uint8_t payload_len);
void vw_uds_query_dsg_tp20();
void vw_uds_query_haldex_uds();

// Process incoming response frames, including DSG/Haldex UDS wheel speed data
void vw_uds_process_response(const twai_message_t &frame);

#ifdef __cplusplus
}
#endif

#endif // VW_UDS_H