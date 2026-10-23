#include "can2cluster_gateway.h"
#include "can2cluster_savvycan.h"

// The byte values below are the ones the customer's MK4 build (v4.01.2) sends.

// True when another module sent this ID in the last second (so we stay quiet on it)
static bool isOtherSender(uint32_t otherSenderMs, uint32_t now)
{
  if (otherSenderMs != 0 && (now - otherSenderMs) < GATEWAY_OTHER_SENDER_MS)
  {
    return true;
  }
  return false;
}

// True when the cluster sent its mileage (Kombi_3 0x520) in the last 5 s
bool isClusterKmFresh()
{
  uint32_t seenMs = clusterKmSeenMs; // plain copy - the CAN task changes it
  if (seenMs != 0 && (millis() - seenMs) < GATEWAY_KM_TIMEOUT_MS)
  {
    return true;
  }
  return false;
}

// Put one frame on the bus, and copy it into the SavvyCAN stream like the paddle frames
static void sendGatewayFrame(twai_message_t frame)
{
  if (twai_transmit(&frame, pdMS_TO_TICKS(5)) == ESP_OK)
  {
    analyzerQueueFrame(frame, 0);
  }
}

// 0x570 BSG_Last (power management), 4 bytes
static void sendBsgLast()
{
  bool isEngineRunning = false;
  if (vehicleRPMCAN > ENGINE_RUNNING_RPM)
  {
    isEngineRunning = true;
  }

  twai_message_t frame{}; // {} = start with every part of the frame at zero
  frame.identifier = BSG_LAST_ID;
  frame.data_length_code = 4;
  if (isEngineRunning)
  {
    frame.data[0] = 0x82; // bit 1 = ignition on (terminal 15), bit 7 = engine running
    frame.data[2] = 176;  // battery voltage 13.8 V (byte = (volts - 5) / 0.05), alternator charging
  }
  else
  {
    frame.data[0] = 0x02; // bit 1 = ignition on (terminal 15)
    frame.data[2] = 148;  // battery voltage 12.4 V (byte = (volts - 5) / 0.05)
  }
  frame.data[1] = 0x00; // not used
  frame.data[3] = 0x00; // not used
  sendGatewayFrame(frame);
}

// 0x390 Gate_Komf_1 (comfort gateway), 8 bytes
static void sendGateKomf1()
{
  twai_message_t frame{}; // {} = every byte starts at 0
  frame.identifier = GATE_KOMF_1_ID;
  frame.data_length_code = 8;
  if (vehicleReverse)
  {
    frame.data[3] = 0x10; // byte 3 bit 4 = reverse gear engaged
  }
  sendGatewayFrame(frame);
}

// 0x7D0 Diagnose_1 (diagnostic data), 8 bytes: the mileage from the cluster
static void sendDiagnose1()
{
  bool isKmValid = isClusterKmFresh();
  uint32_t km = 0; // 0 km sent until the cluster tells us the real mileage
  if (isKmValid)
  {
    km = clusterKm;
  }

  twai_message_t frame{}; // {} = every byte starts at 0
  frame.identifier = DIAGNOSE_1_ID;
  frame.data_length_code = 8;
  frame.data[0] = 0x00;                    // not used
  frame.data[1] = km & 0xFF;               // mileage bits 0-7
  frame.data[2] = (km >> 8) & 0xFF;        // mileage bits 8-15
  frame.data[3] = (km >> 16) & 0x0F;       // mileage bits 16-19 (low 4 bits)
  frame.data[4] = 0x88;                    // fixed value from the customer's build
  frame.data[5] = 0x00;                    // not used
  frame.data[6] = 0x00;                    // not used
  if (isKmValid)
  {
    frame.data[7] = 0x80;                  // bit 7 set, bit 6 clear = mileage is real
  }
  else
  {
    frame.data[7] = 0xC0;                  // bit 7 set, bit 6 set = mileage not available
  }
  sendGatewayFrame(frame);
}

// Task: sends the gateway frames while "Send PQ Gateway Frames" is ticked
void gatewayFramesTask(void *args)
{
  uint32_t lastFastMs = 0; // millis() when 0x570 / 0x390 last went out
  uint32_t lastSlowMs = 0; // millis() when 0x7D0 last went out

  while (1)
  {
    if (!useGatewayFrames)
    {
      vTaskDelay(pdMS_TO_TICKS(100)); // switched off: just check again in a bit
      continue;
    }

    uint32_t now = millis();

    // 0x570 and 0x390 every 100 ms
    if ((now - lastFastMs) >= GATEWAY_FAST_MS)
    {
      lastFastMs = now;
      if (!isOtherSender(bsgLastOtherSenderMs, now))
      {
        sendBsgLast();
      }
      if (!isOtherSender(gateKomf1OtherSenderMs, now))
      {
        sendGateKomf1();
      }
    }

    // 0x7D0 every 500 ms
    if ((now - lastSlowMs) >= GATEWAY_SLOW_MS)
    {
      lastSlowMs = now;
      if (!isOtherSender(diagnose1OtherSenderMs, now))
      {
        sendDiagnose1();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// A gateway-related frame arrived from another module (the CAN chip never hands us our own)
void onGatewayFrameRX(const twai_message_t &frame)
{
  uint32_t now = millis();

  if (frame.identifier == BSG_LAST_ID)
  {
    bsgLastOtherSenderMs = now; // someone else sends 0x570: we go quiet on it
  }
  else if (frame.identifier == GATE_KOMF_1_ID)
  {
    gateKomf1OtherSenderMs = now; // someone else sends 0x390
  }
  else if (frame.identifier == DIAGNOSE_1_ID)
  {
    diagnose1OtherSenderMs = now; // someone else sends 0x7D0
  }
  else if (frame.identifier == KOMBI_3_ID && frame.data_length_code >= 8)
  {
    // Kombi_3 from the cluster: mileage in km, 20 bits across bytes 5-7
    uint32_t km = frame.data[5];                   // byte 5 = mileage bits 0-7
    km = km | ((uint32_t)frame.data[6] << 8);      // byte 6 = mileage bits 8-15
    km = km | ((uint32_t)(frame.data[7] & 0x0F) << 16); // byte 7 low 4 bits = mileage bits 16-19
    if (km >= 1 && km <= CLUSTER_KM_MAX)
    {
      clusterKm = km;
      clusterKmSeenMs = now;
      if (clusterKmSeenMs == 0)
      {
        clusterKmSeenMs = 1; // 0 means "never seen"
      }
    }
  }
}

// What the web page shows for one gateway frame
uint8_t gatewayFrameState(uint32_t otherSenderMs)
{
  if (!useGatewayFrames)
  {
    return GATEWAY_STATE_OFF;
  }
  if (isOtherSender(otherSenderMs, millis()))
  {
    return GATEWAY_STATE_OTHER;
  }
  return GATEWAY_STATE_SENDING;
}
