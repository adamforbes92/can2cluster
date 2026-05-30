#include "can2cluster_can.h"
#include "can2cluster_uds.h"
#include "can2cluster_savvycan.h"

void canInit()
{
  // Configure TWAI (CAN) controller
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)pinTX_CAN, (gpio_num_t)pinRX_CAN, TWAI_MODE_NORMAL);
  g_config.rx_queue_len = 256;
  // Enable alerts so we can detect (and recover from) error-passive / bus-off events.
  g_config.alerts_enabled = TWAI_ALERT_BUS_OFF
                          | TWAI_ALERT_BUS_RECOVERED
                          | TWAI_ALERT_ERR_PASS
                          | TWAI_ALERT_ABOVE_ERR_WARN
                          | TWAI_ALERT_RX_QUEUE_FULL;
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
  {
    DEBUG("TWAI driver install failed");
    return;
  }

  // Start TWAI driver
  if (twai_start() != ESP_OK)
  {
    DEBUG("TWAI start failed");
    return;
  }

  // Create a task to handle incoming CAN messages
  xTaskCreate(canReceiveTask, "canReceiveTask", 4096, NULL, 5, NULL);

  // Create a task to watch TWAI alerts and recover from bus-off
  xTaskCreate(canMonitorTask, "canMonitorTask", 3072, NULL, 4, NULL);
}

void canReceiveTask(void *args)
{
  twai_message_t rx_frame;
  while (1)
  {
    while (twai_receive(&rx_frame, 0) == ESP_OK)
    {
      onBodyRX(rx_frame);
      // Forward to SavvyCAN analyzer (no-op when analyzerMode and analyzerSerial are both false)
      analyzerQueueFrame(rx_frame, 0);
      // Route TP2.0 frames to the TP2.0 task queue
      if (tp20RxQueue &&
          (rx_frame.identifier == TP20_DSG_SETUP_RX ||
           rx_frame.identifier == TP20_RX_ID)) {
        xQueueSendToBack(tp20RxQueue, &rx_frame, 0);
      }
      // Route UDS response frames to the UDS task queue
      if (udsRxQueue && rx_frame.identifier == UDS_RX_ID) {
        xQueueSendToBack(udsRxQueue, &rx_frame, 0);
      }
    }
    vTaskDelay(1);
  }
}

// Monitors TWAI alerts and drives bus-off recovery.
//
// Sequence on bus-off:
//   1. TWAI_ALERT_BUS_OFF fires.
//   2. We call twai_initiate_recovery(), which puts the controller into
//      "recovering" state. The controller waits for 128 occurrences of
//      11 consecutive recessive bits before returning to "stopped".
//   3. TWAI_ALERT_BUS_RECOVERED fires when that completes.
//   4. We call twai_start() to bring the driver back online.
void canMonitorTask(void *args)
{
  while (1)
  {
    uint32_t alerts = 0;
    // Block up to 1 s waiting for an alert. read_alerts also clears the bits.
    if (twai_read_alerts(&alerts, pdMS_TO_TICKS(1000)) != ESP_OK)
    {
      continue;
    }

    if (alerts & TWAI_ALERT_ABOVE_ERR_WARN)
    {
      DEBUG("TWAI: error counter above warning level");
    }
    if (alerts & TWAI_ALERT_ERR_PASS)
    {
      DEBUG("TWAI: error-passive state entered");
    }
    if (alerts & TWAI_ALERT_RX_QUEUE_FULL)
    {
      DEBUG("TWAI: RX queue full — frames dropped");
    }
    if (alerts & TWAI_ALERT_BUS_OFF)
    {
      DEBUG("TWAI: BUS-OFF — initiating recovery");
      hasCAN = false;
      esp_err_t r = twai_initiate_recovery();
      if (r != ESP_OK)
      {
        DEBUG("TWAI: twai_initiate_recovery() returned %d", (int)r);
      }
    }
    if (alerts & TWAI_ALERT_BUS_RECOVERED)
    {
      DEBUG("TWAI: bus recovered — restarting driver");
      esp_err_t r = twai_start();
      if (r != ESP_OK)
      {
        DEBUG("TWAI: twai_start() after recovery returned %d", (int)r);
      }
    }
  }
}

void onBodyRX(const twai_message_t &frame)
{
#if ChassisCANDebug // print incoming CAN messages
  DEBUG_CHASSIS_CAN_("Length: %u ID: 0x%03X Buffer: ", frame.data_length_code, frame.identifier);
  for (uint8_t i = 0; i < frame.data_length_code; i++)
  {
    if (i > 0)
      Serial.print(" ");
    Serial.printf("%02X", frame.data[i]);
  }
  Serial.println();
#endif
  lastCAN = millis();
  hasCAN = true;

  switch (frame.identifier)
  {
  case MOTOR1_ID:
    // frame[2] (byte 3) > motor speed low byte
    // frame[3] (byte 4) > motor speed high byte
    // frame[4] (byte 3) > khm speed?
    vehicleRPMCAN = ((frame.data[3] << 8) | frame.data[2]) * 0.25; // conversion: 0.25*HEX
    break;

  case MOTOR2_ID:
    ecuSpeed = (frame.data[3] * 100 * 128) / 10000;
    break;

  case MOTOR5_ID:
    // set EML & EPC based on the bit read (LSB, so backwards)
    vehicleEML = bitRead(frame.data[1], 5);
    vehicleEPC = bitRead(frame.data[1], 6);
    break;

  case MOTOR6_ID:
    if (frame.data[0] == 0x73 || frame.data[0] == 0x72)
    {
      // vehicleReverse = true;
    }
    else
    {
      // vehicleReverse = false;
    }
    // Note: vehiclePark is authoritatively managed by gearLever_ID (0x448).
    // Only assert park from this frame; never clear it here to avoid racing.
    if (frame.data[0] == 0x83 || frame.data[0] == 0x82)
    {
      vehiclePark = true;
    }
    break;

  case BRAKES3_ID:
    if (1)
    {
      const uint16_t br3_speed_raw = (((uint16_t)frame.data[1] << 8) | frame.data[0]) >> 1;
      absSpeed = (uint16_t)(br3_speed_raw * 0.01f + 0.5f);
    }
    break;

  case mWaehlhebel_1_ID:
    gear_raw = ((frame.data[7] & 0b01110000) >> 4) - 1;
    lever_raw = (frame.data[7] & 0b00000001);

    if (lever_raw)
    {
      gear = gear_raw;
      switch (gear)
      {
      case 3:
        break;
      default:
        break;
      }

      if (gear == 0xFF)
      {
        gear = 1;
      }
    }
    break;

  case gearLever_ID:
    lever = (frame.data[0] & 0b11110000) >> 4;
    switch (lever)
    {
    case LEVER_P:
      vehiclePark = true;
      vehicleReverse = false;
      vehicleNeutral = false;
      break;
    case LEVER_R:
      vehiclePark = false;
      vehicleReverse = true;
      vehicleNeutral = false;
      break;
    case LEVER_N:
      vehiclePark = false;
      vehicleReverse = false;
      vehicleNeutral = true;
      break;
    case LEVER_D:
    case LEVER_S:
    case LEVER_TIPTRONIC_ON:
    case LEVER_TIPTRONIC_UP:
    case LEVER_TIPTRONIC_DOWN:
      vehiclePark = false;
      vehicleReverse = false;
      vehicleNeutral = false;
      break;
    }
    break;

  case emeraldECU1_ID:
    vehicleRPMCAN = ((frame.data[0] << 8) | frame.data[1]);
    break;

  case emeraldECU2_ID:
    calcSpeed = ((frame.data[2] << 8) | frame.data[3]) * (2.25 / 256);
    break;

  case fordECU1_ID:
    vehicleRPMCAN = frame.data[1] & 0x00FF;
    vehicleRPMCAN |= (frame.data[0] << 8) & 0x7F00;
    break;

  case fordECU2_ID:
    vehicleOilPressure = (frame.data[5] >> 4) & 0x01;
    vehicleBattLight = (frame.data[4] >> 1) & 0x01;
    vehicleEML = (frame.data[4] >> 6) & 0x03;
    vehicleCoolantTemp = (frame.data[0] & 0xFF) - 40;
    break;

  default:
    // do nothing...
    break;
  }

  // Aftermarket / Custom CAN speed input — parsed independently of the VW switch table
  if (useAftermarket && frame.identifier == (aftermarketSpeedID & 0x7FF)) {
    uint8_t lowIdx  = constrain(aftermarketSpeedLowByte,  0, 7);
    uint8_t highIdx = constrain(aftermarketSpeedHighByte, 0, 7);
    uint16_t rawValue;
    if (aftermarketSpeedLittleEndian) {
      // LSB at lowIdx, MSB at highIdx
      rawValue = (uint16_t)frame.data[lowIdx] | ((uint16_t)frame.data[highIdx] << 8);
    } else {
      // Big-endian: MSB at lowIdx, LSB at highIdx (mirrors broadcastSpeed convention)
      rawValue = (uint16_t)frame.data[highIdx] | ((uint16_t)frame.data[lowIdx] << 8);
    }
    double scaled = (rawValue * aftermarketSpeedScale) + aftermarketSpeedOffset;
    aftermarketSpeed = scaled < 0.0 ? 0.0 : scaled;
  }
}

void sendPaddleUpFrame()
{
  twai_message_t paddlesUp{}; // 0x7C0
  paddlesUp.identifier = GRA_ID;
  paddlesUp.data_length_code = 4;
  paddlesUp.data[0] = 0x0E;     // was 0xB7 chksum = byte 2 XOR byte 3 XOR byte 4
  paddlesUp.data[2] = 0x0C;     // was 0x34
  paddlesUp.data[3] = 0x02;     //
  bitSet(paddlesUp.data[3], 1); // set high (trigger)
  if (twai_transmit(&paddlesUp, pdMS_TO_TICKS(100)) != ESP_OK)
  {
    // failed, ignore
  }
}

void sendPaddleDownFrame()
{
  twai_message_t paddlesDown{}; // 0x7C0
  paddlesDown.identifier = GRA_ID;
  paddlesDown.data_length_code = 4;
  paddlesDown.data[0] = 0x0D; // chksum = byte 2 XOR byte 3 XOR byte
  paddlesDown.data[2] = 0x0C;
  paddlesDown.data[3] = 0x01;
  bitSet(paddlesDown.data[3], 1); // set high (trigger)
  if (twai_transmit(&paddlesDown, pdMS_TO_TICKS(100)) != ESP_OK)
  {
    // failed, ignore
  }
}

void broadcastGRA(void *args)
{
  uint8_t activeGraCommand = 0x00;
  uint32_t activeGraCommandUntilMs = 0;

  while (1)
  {
#if detailedDebugStack
    stackbroadcastGRA = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

    uint8_t graPulseMS = 80; // how long to hold the paddle signal high for (ms)

    twai_message_t broadcastGRA{};
    broadcastGRA.identifier = GRA_ID;
    broadcastGRA.data_length_code = 4;
    // broadcastGRA.data[0] = GRA_crc; - calculated soon...
    broadcastGRA.data[1] = 0x00;        // always zero
    broadcastGRA.data[2] = GRA_counter; // full 8-bit rolling counter (0x00 > 0xFF)
    if (padUpTxPending && padDownTxPending)
    {
#if serialDebugPaddles
      DEBUG("Paddle up/down triggered together");
#endif
      padUpTxPending = false;
      padDownTxPending = false;
    }
    else if (padUpTxPending)
    {
#if serialDebugPaddles
      DEBUG("Paddle up");
#endif
      activeGraCommand = 0x02;
      activeGraCommandUntilMs = millis() + graPulseMS;
      padUpTxPending = false;
    }
    else if (padDownTxPending)
    {
#if serialDebugPaddles
      DEBUG("Paddle down");
#endif
      activeGraCommand = 0x01;
      activeGraCommandUntilMs = millis() + graPulseMS;
      padDownTxPending = false;
    }

    if (activeGraCommand != 0x00 && (int32_t)(millis() - activeGraCommandUntilMs) < 0)
    {
      broadcastGRA.data[3] = activeGraCommand;
    }
    else
    {
      broadcastGRA.data[3] = 0x00;
      activeGraCommand = 0x00;
    }

    GRA_crc = 0;
    for (uint8_t i = 2; i < 5; i++)
    {
      GRA_crc ^= broadcastGRA.data[i]; // xor byte 2, 3, 4
    }
    broadcastGRA.data[0] = GRA_crc;

    if (twai_transmit(&broadcastGRA, pdMS_TO_TICKS(100)) != ESP_OK)
    { // write CAN frame from the body to the Haldex
    }

    GRA_counter++;
    vTaskDelay(pdMS_TO_TICKS(broadcastGRARefresh));
  }
}

void broadcastSpeed(void *args)
{
  while (1)
  {
#if detailedDebugStack
    stackbroadcastSpeed = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

    if (!broadcastSpeedEnabled)
    {
      vTaskDelay(pdMS_TO_TICKS(broadcastSpeedRefresh));
      continue;
    }

    twai_message_t speedFrame{};
    speedFrame.identifier = broadcastSpeedID & 0x7FF;
    speedFrame.extd = 0;
    speedFrame.data_length_code = constrain(broadcastSpeedDLC, 0, 8);

    for (uint8_t i = 0; i < 8; i++)
    {
      speedFrame.data[i] = broadcastSpeedData[i];
    }

    int32_t scaledSpeed = static_cast<int32_t>((vehicleSpeed * broadcastSpeedScale) + broadcastSpeedOffset);
    scaledSpeed = constrain(scaledSpeed, 0, 65535);
    broadcastSpeedValue = static_cast<uint16_t>(scaledSpeed);

    uint8_t lowByteIndex = constrain(broadcastSpeedLowByte, 0, 7);
    uint8_t highByteIndex = constrain(broadcastSpeedHighByte, 0, 7);
    uint8_t lowByte = static_cast<uint8_t>(broadcastSpeedValue & 0xFF);
    uint8_t highByte = static_cast<uint8_t>((broadcastSpeedValue >> 8) & 0xFF);

    if (broadcastSpeedLittleEndian)
    {
      speedFrame.data[lowByteIndex] = lowByte;
      speedFrame.data[highByteIndex] = highByte;
    }
    else
    {
      speedFrame.data[lowByteIndex] = highByte;
      speedFrame.data[highByteIndex] = lowByte;
    }

#if serialDebugIO
    DEBUG_IO_("TX SPEED ID:0x%03X DLC:%u Data:", speedFrame.identifier, speedFrame.data_length_code);
    for (uint8_t i = 0; i < speedFrame.data_length_code; i++)
    {
      DEBUG_IO_(" %02X", speedFrame.data[i]);
    }
    DEBUG_IO(" Value:%u (vehicleSpeed:%u)", broadcastSpeedValue, vehicleSpeed);
#endif

    if (twai_transmit(&speedFrame, pdMS_TO_TICKS(5)) != ESP_OK)
    {
      // frame dropped if TX queue is full
    }

    vTaskDelay(pdMS_TO_TICKS(broadcastSpeedRefresh));
  }
}