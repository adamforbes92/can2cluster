#include "vw_uds.h"

bool vw_uds_send_request(uint32_t id, const uint8_t* payload, uint8_t payload_len) {
#if !VW_UDS_ENABLED
  // UDS is disabled — never put a frame on the bus from here. See vw_uds.h.
  (void)id; (void)payload; (void)payload_len;
  return false;
#else
  twai_message_t req{};
  req.identifier = id;
  req.extd = false;
  req.data_length_code = payload_len > 8 ? 8 : payload_len;

  for (uint8_t i = 0; i < req.data_length_code; i++) {
    req.data[i] = payload[i];
  }

  // Pad unused bytes for clarity
  for (uint8_t i = req.data_length_code; i < 8; i++) {
    req.data[i] = 0x00;
  }

  if (twai_transmit(&req, pdMS_TO_TICKS(100)) != ESP_OK) {
    return false;
  }
  return true;
#endif
}

void vw_uds_query_dsg_tp20() {
#if !VW_UDS_ENABLED
  return;
#else
  uint8_t payload[8] = {0};
  payload[0] = 0x02;  // ISO-TP single frame length=2
  payload[1] = 0x10;  // StartDiagnosticSession
  payload[2] = 0x03;  // Extended diagnostic session

  if (vw_uds_send_request(mWaehlhebel_1_ID, payload, 3)) {
    DEBUG_UDS("DSC query to DSG sent (TP2.0 StartSession)");
  } else {
    DEBUG_UDS("DSG query send failed");
  }
#endif
}

void vw_uds_query_haldex_uds() {
#if !VW_UDS_ENABLED
  return;
#else
  uint8_t payload[8] = {0};
  payload[0] = 0x03;  // ISO-TP single frame length=3
  payload[1] = 0x22;  // ReadDataByIdentifier
  payload[2] = 0xF1;  // Example DID high
  payload[3] = 0x90;  // Example DID low (DSG wheel speed candidate)

  if (vw_uds_send_request(HALDEX_ID, payload, 4)) {
    DEBUG_UDS("Haldex UDS DID request sent");
  } else {
    DEBUG_UDS("Haldex query send failed");
  }
#endif
}

void vw_uds_process_response(const twai_message_t &frame) {
#if !VW_UDS_ENABLED
  (void)frame;
  return;
#else
  if (frame.identifier != mWaehlhebel_1_ID && frame.identifier != HALDEX_ID) {
    return;
  }

  if (frame.data_length_code < 2) {
    return;
  }

  uint8_t responseSID = frame.data[1];
  if (responseSID == 0x50) {
    DEBUG_UDS("0x%03X positive StartSession", frame.identifier);
    return;
  }

  if (responseSID == 0x62 && frame.data_length_code >= 6) {
    uint16_t did = (frame.data[2] << 8) | frame.data[3];
    uint16_t raw = (frame.data[4] << 8) | frame.data[5];
    double speed = raw / 100.0; // Assumes 0.01 scale per 0xF190

    if (frame.identifier == mWaehlhebel_1_ID) {
      dsgUDSSpeed = speed;
      DEBUG_UDS("DSG UDS DID 0x%04X speed=%.2f km/h", did, speed);
    } else {
      haldexUDSSpeed = speed;
      DEBUG_UDS("Haldex UDS DID 0x%04X speed=%.2f km/h", did, speed);
    }

    if (dsgUDSSpeed > 0) {
      vehicleSpeed = (uint16_t)round(dsgUDSSpeed);
    } else if (haldexUDSSpeed > 0) {
      vehicleSpeed = (uint16_t)round(haldexUDSSpeed);
    }
    return;
  }

  if (responseSID == 0x7F && frame.data_length_code >= 3) {
    uint8_t negCode = frame.data[2];
    DEBUG_UDS("0x%03X negative response: 0x%02X", frame.identifier, negCode);
    return;
  }
#endif // VW_UDS_ENABLED
}
