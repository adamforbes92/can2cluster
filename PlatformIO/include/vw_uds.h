#ifndef VW_UDS_H
#define VW_UDS_H

#include "can2cluster_defs.h"
#include <driver/twai.h>

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