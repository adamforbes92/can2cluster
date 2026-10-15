  #include "can2cluster_can.h"
#include "can2cluster_uds.h"
#include "can2cluster_savvycan.h"

void canInit()
{
  // Chassis CAN pins (DevKit or S3 numbers, picked in _defs.h)
  const gpio_num_t txPin = (gpio_num_t)pinTX_CAN;
  const gpio_num_t rxPin = (gpio_num_t)pinRX_CAN;

  // Configure TWAI (CAN) controller
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL);
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
    DEBUG_CAN("TWAI driver install failed");
    return;
  }

  // Start TWAI driver
  if (twai_start() != ESP_OK)
  {
    DEBUG_CAN("TWAI start failed");
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
//   1. The controller goes bus-off (TWAI_ALERT_BUS_OFF fires).
//   2. We call twai_initiate_recovery(), which puts the controller into
//      "recovering" state. The controller waits for 128 occurrences of
//      11 consecutive recessive bits before returning to "stopped".
//   3. TWAI_ALERT_BUS_RECOVERED fires when that completes.
//   4. We call twai_start() to bring the driver back online.
//
// The alerts only wake the task; what it does comes from the controller state,
// read on every pass (at least once a second), so a missed alert or a failed
// call is retried instead of leaving CAN dead until a power cycle. Nothing in
// the firmware stops the controller on purpose, so a stopped one is always restarted.
void canMonitorTask(void *args)
{
  bool wasBusOff = false;
  while (1)
  {
    uint32_t alerts = 0;
    // Block up to 1 s waiting for an alert. read_alerts also clears the bits.
    if (twai_read_alerts(&alerts, pdMS_TO_TICKS(1000)) == ESP_OK)
    {
      if (alerts & TWAI_ALERT_ABOVE_ERR_WARN)
      {
        DEBUG_CAN("TWAI: error counter above warning level");
      }
      if (alerts & TWAI_ALERT_ERR_PASS)
      {
        DEBUG_CAN("TWAI: error-passive state entered");
      }
      if (alerts & TWAI_ALERT_RX_QUEUE_FULL)
      {
        DEBUG_CAN("TWAI: RX queue full — frames dropped");
      }
    }

    twai_status_info_t st;
    if (twai_get_status_info(&st) != ESP_OK)
    {
      continue;
    }
    if (st.state == TWAI_STATE_BUS_OFF)
    {
      if (!wasBusOff)
      {
        DEBUG_CAN("TWAI: BUS-OFF — initiating recovery");
      }
      wasBusOff = true;
      hasCAN = false;
      esp_err_t r = twai_initiate_recovery();
      if (r != ESP_OK)
      {
        DEBUG_CAN("TWAI: twai_initiate_recovery() returned %d", (int)r);
      }
    }
    else if (st.state == TWAI_STATE_STOPPED)
    {
      esp_err_t r = twai_start();
      if (r == ESP_OK)
      {
        DEBUG_CAN("TWAI: %s — driver restarted", wasBusOff ? "bus recovered" : "controller was stopped");
        wasBusOff = false;
      }
      else
      {
        DEBUG_CAN("TWAI: twai_start() returned %d", (int)r);
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

  case MOTOR3_ID:
    // Coolant temperature source. TODO(user): confirm the CAN ID, byte index
    // and scaling for your vehicle — this is a placeholder using the VW-style
    // "raw byte 0 minus 40 = degrees C" convention, mirroring how RPM is fixed.
    vehicleCoolantTemp = frame.data[0] - 40;
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

  case RENAULT_MEGANE_ECU:
    // Renault Megane engine RPM: bytes 0..1 big-endian, raw/8 (verified against
    // 2K/3K/4K/5K RPM logs — 4000 RPM reads 0x7D00).
    vehicleRPMCAN = (((uint16_t)frame.data[0] << 8) | frame.data[1]) >> 3;
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

  case MOTOR_12:
  {
    // MQB Motor_12 (0x0A8) - SG_ MO_Drehzahl_01 : 48|16@1+ (0.25,0) [0|16383] "Unit_MinutInver"
    // i.e. engine speed (Drehzahl = revs) in RPM, factor 0.25/bit, 16-bit LE, bytes 6+7.
    const uint16_t mo_drehzahl_raw = ((uint16_t)frame.data[7] << 8) | frame.data[6];
    vehicleRPMCAN = (uint16_t)(mo_drehzahl_raw * 0.25f + 0.5f);
    break;
  }

  case MOTOR_07:
  {
    // MQB Motor_07 (0x640) - SG_ MO_Kuehlmitteltemp : 24|8@1+ (0.75,-48) "degC"  -> byte 3, coolant temperature
    const int16_t coolant_c = (int16_t)(frame.data[3] * 0.75f - 48.0f + 0.5f);
    vehicleCoolantTemp = (coolant_c > 0) ? (uint8_t)coolant_c : 0;
    break;
  }

  case MOTOR_18:
  {
    // MQB Motor_18 (0x670) - SG_ MO_EPCL : 13|3@1+  -> byte 1 bits 5..7, EPC lamp request (0 = off, >0 = lamp/text variants)
    // MQB has no engine MIL (EML) broadcast on FCAN - the check-engine
    // lamp is driven via the OBD/diagnostic path only, so vehicleEML has no MQB source.
    vehicleEPC = ((frame.data[1] >> 5) & 0x07) != 0;
    break;
  }

  case ESP_19:
  {
    // MQB ESP_19 (0x0B2) - per-wheel ABS speeds broadcast.
    // MQB FCAN K-matrix: four 16-bit LE wheel speeds (HL/HR/VL/VR) with
    // factor 0.0075 km/h per bit each.
    //   HL = Hinten Links  (rear left)   -> bytes 0..1
    //   HR = Hinten Rechts (rear right)  -> bytes 2..3
    //   VL = Vorne  Links  (front left)  -> bytes 4..5
    //   VR = Vorne  Rechts (front right) -> bytes 6..7
    // Average all four for a stable vehicle speed (ABS speed source).
    const uint16_t wheel_speed_hl_raw = (frame.data[1] << 8) | frame.data[0];
    const uint16_t wheel_speed_hr_raw = (frame.data[3] << 8) | frame.data[2];
    const uint16_t wheel_speed_vl_raw = (frame.data[5] << 8) | frame.data[4];
    const uint16_t wheel_speed_vr_raw = (frame.data[7] << 8) | frame.data[6];
    absSpeed = (wheel_speed_hl_raw + wheel_speed_hr_raw + wheel_speed_vl_raw + wheel_speed_vr_raw) * (0.0075 / 4.0);
    break;
  }

  case ESP_21:
  {
    // MQB ESP_21 (0x0FD) - ESP/ASR mode + reference speed.
    // MQB FCAN K-matrix:
    //   SG_ ESP_v_Signal : 16@1+ (0.01,0) "km/h"  -> bytes 4..5 LE, vehicle reference speed
    // Mapped to the ECU speed source so MQB works with either speed selection.
    ecuSpeed = (((uint16_t)frame.data[5] << 8) | frame.data[4]) * 0.01;
    break;
  }

  case GETRIEBE_11:
  {
    // MQB Getriebe_11 (0x0AD) - DSG status broadcast.
    //   SG_ GE_Fahrstufe : 42|4@1+  -> byte 5 bits 2..5, gear-lever position
    // Values: 0=between positions, 5=P, 6=R, 7=N, 8=D, 9=S, 10=E, 13=tip from S,
    // 14=tip from D (M gate, or paddles pulled in D). V4.01: 14 and 10 were
    // missing, so the gear froze in the M gate / while using the paddles in D.
    //   SG_ GE_Zielgang  -> byte 7 high nibble, target/engaged gear (1..7, 0 = none)
    // Verified against the OpenHaldex MQB logs "gears all inc tip and sport.csv"
    // (13), "gears all inc tip.csv" and "drive to fourth, sport, tc, etc.csv" (14).
    // Also feeds the PQ 'lever'/'gear' globals so the DSG speed source works on MQB.
    const uint8_t fahrstufe = (frame.data[5] >> 2) & 0x0F;
    const uint8_t zielgang = (frame.data[7] >> 4) & 0x0F;
    switch (fahrstufe)
    {
    case MQB_FAHRSTUFE_P:
      vehiclePark = true;
      vehicleReverse = false;
      vehicleNeutral = false;
      lever = LEVER_P;
      gear = 0;
      break;
    case MQB_FAHRSTUFE_R:
      vehiclePark = false;
      vehicleReverse = true;
      vehicleNeutral = false;
      lever = LEVER_R;
      gear = 0;
      break;
    case MQB_FAHRSTUFE_N:
      vehiclePark = false;
      vehicleReverse = false;
      vehicleNeutral = true;
      lever = LEVER_N;
      gear = 0;
      break;
    case MQB_FAHRSTUFE_D:
    case MQB_FAHRSTUFE_E:
    case MQB_FAHRSTUFE_S:
    case MQB_FAHRSTUFE_TIP:
    case MQB_FAHRSTUFE_TIP_D:
      vehiclePark = false;
      vehicleReverse = false;
      vehicleNeutral = false;
      if (fahrstufe == MQB_FAHRSTUFE_D || fahrstufe == MQB_FAHRSTUFE_E)
      {
        lever = LEVER_D;
      }
      else if (fahrstufe == MQB_FAHRSTUFE_S)
      {
        lever = LEVER_S;
      }
      else
      {
        lever = LEVER_TIPTRONIC_ON; // manual: lever in the M gate, or a paddle pulled in D/S
      }
      if (zielgang >= 1 && zielgang <= 6)
      {
        gear = zielgang;
      }
      else
      {
        gear = 0;
      }
      break;
    default:
      // init / unknown - leave current state untouched
      break;
    }
    break;
  }

  case MQB_SELECTOR_ID:
  {
    // MQB selector lever (Waehlhebel_03). Read only, for the web page - we
    // never send this ID (see MQB_SELECTOR_ID in _defs.h).
    // byte 0 top 4 bits = lever position (14 = M gate, 10 = Tip+, 11 = Tip-)
    // byte 1 top 4 bits = the same position inverted, as a check
    if (frame.data_length_code >= 2)
    {
      uint8_t leverPosition = frame.data[0] >> 4;         // byte 0 bits 4-7: WH_SensorPos_roh
      uint8_t leverPositionInverted = frame.data[1] >> 4; // byte 1 bits 4-7: WH_SensorPos_roh_inv
      if ((leverPosition + leverPositionInverted) == 15)  // the two always add up to 15 in a good frame
      {
        mqbLeverPosition = leverPosition;
      }
    }
    break;
  }

  case GETRIEBE_17:
  {
    // MQB Getriebe_17 (0x0B1) - gearbox tip mode, read for the web page.
    // byte 2 bits 0-1 = GE_TippZustand: 0 none, 1 M gate, 2 temporary from a paddle
    mqbTipState = frame.data[2] & 0x03;
    break;
  }

  case WBA_03:
  {
    // MQB WBA_03 (0x394) - gearbox -> cluster display, read for the web page.
    // byte 2 bits 4-7 = WBA_GE_Warnung_02: 0 none, 1 error level I, 2 level II, ...
    mqbGearboxWarning = frame.data[2] >> 4;
    break;
  }

  case MQB_PADDLE_ID:
  {
    // A 0x3DD we didn't send (the CAN chip never hands us our own frames), so
    // a real gateway is on the bus. broadcastPaddles() then stays quiet on
    // 0x3DD rather than fight it over the paddle bits.
    mqbPaddleOtherSenderMs = millis();
    break;
  }

  case GRA_ID:
  {
    // The same for PQ: a 0x38A we didn't send means a real gateway is already
    // passing the steering column's cruise/paddle frame onto this bus.
    // broadcastPaddles() then stays quiet on 0x38A - two senders would make the
    // counter jump about and the paddle bits flicker.
    graPaddleOtherSenderMs = millis();
    break;
  }

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

// Send one MQB Gateway_74 (0x3DD) frame. Byte 7 carries the paddles. Bytes 0-6
// are the values a real MQB gateway sent in the customer's log - they're for
// other modules (start-stop, climate, ACC); the gearbox only reads byte 0 bit 2
// and byte 7. Byte map from the K-matrix, see MQB_PADDLES.md.
void sendMqbPaddleFrame(uint8_t paddleBits)
{
  twai_message_t mqbPaddle{}; // {} = start with every part of the frame at zero
  mqbPaddle.identifier = MQB_PADDLE_ID;
  mqbPaddle.data_length_code = 8;
  mqbPaddle.data[0] = 0x08;       // "old data" flags: bit 3 MFG_01_alt = 1. Bit 2 LIN_2_alt = 0 = steering-wheel data is fresh (the gearbox reads this one)
  mqbPaddle.data[1] = 0x04;       // bit 2 ACC_07_alt = 1 (no ACC radar), the rest 0
  mqbPaddle.data[2] = 0x20;       // bits 4-5 EPS_Lenkerposition = 2 (right-hand drive), start-stop info 0
  mqbPaddle.data[3] = 0x00;       // bit 0 MF_Anlassersperre = 0 (starter NOT blocked), start-stop info 0
  mqbPaddle.data[4] = 0xFE;       // ACC_Folgebeschl = 254 "neutral value" (no ACC request)
  mqbPaddle.data[5] = 0x00;       // KL_Geblspng_Soll = 0 (climate fan)
  mqbPaddle.data[6] = 0x0A;       // bits 0-3 recirculation flap = 10, bits 6-7 ACC freewheel info = 0
  mqbPaddle.data[7] = paddleBits; // bit 0 MFL_Tip_Down, bit 1 MFL_Tip_Up, bit 2 LS_Tiptronic_Fehler = 0 (no paddle fault)

  if (twai_transmit(&mqbPaddle, pdMS_TO_TICKS(5)) == ESP_OK)
  {
    analyzerQueueFrame(mqbPaddle, 0); // copy it into the SavvyCAN stream, so a log taken through Can2Cluster shows it too
  }
}

// Send one PQ GRA_Neu (0x38A) frame. Byte 3 carries the paddles. Byte map from
// the VW PQ35/46 K-matrix, see the GRA notes in _defs.h.
void sendGraPaddleFrame(uint8_t paddleBits)
{
  twai_message_t graFrame{}; // {} = start with every part of the frame at zero
  graFrame.identifier = GRA_ID;
  graFrame.data_length_code = 4;
  graFrame.data[1] = 0x00; // cruise buttons: none pressed

  // Byte 2: counter in the top half, sender code in bits 2-3, cruise SET (bit 0) and RESUME (bit 1) left at 0
  uint8_t counterBits = GRA_counter << 4; // counter 0-15 into bits 4-7
  uint8_t senderBits = GRA_SENDER << 2;   // sender code into bits 2-3
  graFrame.data[2] = counterBits | senderBits;

  graFrame.data[3] = paddleBits; // bit 0 = tip down, bit 1 = tip up

  GRA_crc = graFrame.data[1] ^ graFrame.data[2] ^ graFrame.data[3]; // checksum = byte 1 XOR byte 2 XOR byte 3
  graFrame.data[0] = GRA_crc;

  if (twai_transmit(&graFrame, pdMS_TO_TICKS(5)) == ESP_OK)
  {
    analyzerQueueFrame(graFrame, 0); // copy it into the SavvyCAN stream
  }

  // Next counter value: 0-15, then back to 0
  GRA_counter++;
  if (GRA_counter > GRA_COUNTER_MAX)
  {
    GRA_counter = 0;
  }
}

// Paddle task. Every 10 ms it reads the paddle switches and sends them to the
// gearbox in the frame(s) picked on the web page (paddleOutput):
//   MQB Gateway_74 0x3DD - every 100 ms, plus straight away when a paddle changes
//   PQ  GRA_Neu    0x38A - every 20 ms
// A pull is sent for as long as the paddle is held (like the factory parts),
// with a minimum length so a very quick tap still reaches the gearbox.
// It doesn't depend on the speed source any more: "Off" sends nothing.
void broadcastPaddles(void *args)
{
  uint8_t tick = 0;                      // counts 10 ms loops, 0..9 (0 = the 100 ms slot for 0x3DD)
  uint8_t lastMqbBits = MQB_PADDLE_NONE; // paddle bits in the last loop, to spot a change
  bool wasUpHeld = false;                // paddle + on the last loop
  bool wasDownHeld = false;              // paddle - on the last loop
  uint32_t upPulledMs = 0;               // millis() when paddle + was last pulled (0 = not yet)
  uint32_t downPulledMs = 0;             // millis() when paddle - was last pulled (0 = not yet)
  uint8_t testUpLoopsLeft = 0;           // 10 ms loops left of a web page "Test Paddle +" pull
  uint8_t testDownLoopsLeft = 0;         // 10 ms loops left of a web page "Test Paddle -" pull

  while (1)
  {
#if detailedDebugStack
    stackbroadcastPaddles = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

    // --- 1. Read the paddle switches (debounced) ---
    updateButtons();
    uint32_t now = millis();
    bool isUpHeld = isPaddleUpHeld;
    bool isDownHeld = isPaddleDownHeld;

    // The web page "Test Paddle" buttons: take the request, then count it down
    // here as a pull PADDLE_TEST_HOLD_MS long
    if (paddleTestCmd == MQB_PADDLE_UP)
    {
      testUpLoopsLeft = PADDLE_TEST_HOLD_MS / broadcastPaddlesRefresh;
      paddleTestCmd = MQB_PADDLE_NONE; // request taken
    }
    if (paddleTestCmd == MQB_PADDLE_DOWN)
    {
      testDownLoopsLeft = PADDLE_TEST_HOLD_MS / broadcastPaddlesRefresh;
      paddleTestCmd = MQB_PADDLE_NONE; // request taken
    }
    if (testUpLoopsLeft > 0)
    {
      isUpHeld = true;
      testUpLoopsLeft--;
    }
    if (testDownLoopsLeft > 0)
    {
      isDownHeld = true;
      testDownLoopsLeft--;
    }

    // Note the moment each paddle is pulled (for the minimum lengths below)
    if (isUpHeld && !wasUpHeld)
    {
      upPulledMs = now;
#if serialDebugPaddles
      DEBUG_PADDLES("Paddle + pulled");
#endif
    }
    if (isDownHeld && !wasDownHeld)
    {
      downPulledMs = now;
#if serialDebugPaddles
      DEBUG_PADDLES("Paddle - pulled");
#endif
    }
    wasUpHeld = isUpHeld;
    wasDownHeld = isDownHeld;

    // --- 2. Which frames go out (web page setting) ---
    bool sendMQB = false;
    bool sendPQ = false;
    if (paddleOutput == "MQB" || paddleOutput == "All")
    {
      sendMQB = true;
    }
    if (paddleOutput == "PQ" || paddleOutput == "All")
    {
      sendPQ = true;
    }

    // A real gateway is already sending 0x3DD (a factory MQB car, or a swap
    // that kept its gateway): two senders would fight over the paddle bits.
    uint32_t otherSenderMs = mqbPaddleOtherSenderMs;
    if (otherSenderMs != 0 && (now - otherSenderMs) < MQB_PADDLE_OTHER_SENDER_MS)
    {
      sendMQB = false;
    }

    // The same for PQ: a real gateway is already sending 0x38A, so we stay quiet on it
    uint32_t graOtherSenderMs = graPaddleOtherSenderMs;
    if (graOtherSenderMs != 0 && (millis() - graOtherSenderMs) < GRA_OTHER_SENDER_MS)
    {
      sendPQ = false;
    }

    // --- 3. MQB Gateway_74 (0x3DD) ---
    // A paddle counts as pulled while it's held, and for at least
    // MQB_PADDLE_MIN_HOLD_MS after it was pulled (always 2+ frames).
    uint8_t mqbBits = MQB_PADDLE_NONE;
    if (isUpHeld || (upPulledMs != 0 && (now - upPulledMs) < MQB_PADDLE_MIN_HOLD_MS))
    {
      mqbBits = mqbBits | MQB_PADDLE_UP; // byte 7 bit 1: MFL_Tip_Up
    }
    if (isDownHeld || (downPulledMs != 0 && (now - downPulledMs) < MQB_PADDLE_MIN_HOLD_MS))
    {
      mqbBits = mqbBits | MQB_PADDLE_DOWN; // byte 7 bit 0: MFL_Tip_Down
    }

    if (sendMQB)
    {
      // The normal 100 ms slot, or straight away when the bits change (factory gateway behaviour)
      if (tick == 0 || mqbBits != lastMqbBits)
      {
        sendMqbPaddleFrame(mqbBits);
      }
    }
#if serialDebugPaddles
    if (mqbBits != lastMqbBits)
    {
      if (sendMQB)
      {
        DEBUG_PADDLES("Paddle bits now 0x%02X (sent on 0x3DD)", mqbBits);
      }
      else
      {
        DEBUG_PADDLES("Paddle bits now 0x%02X (0x3DD not being sent)", mqbBits);
      }
    }
#endif
    lastMqbBits = mqbBits; // remembered even when 0x3DD is off, so switching it on doesn't send a stale change

    // --- 4. PQ GRA_Neu (0x38A), every 20 ms ---
    uint8_t graBits = MQB_PADDLE_NONE; // same bit values as MQB: bit 0 down, bit 1 up
    if (isUpHeld || (upPulledMs != 0 && (now - upPulledMs) < GRA_PULSE_MIN_MS))
    {
      graBits = graBits | MQB_PADDLE_UP;
    }
    if (isDownHeld || (downPulledMs != 0 && (now - downPulledMs) < GRA_PULSE_MIN_MS))
    {
      graBits = graBits | MQB_PADDLE_DOWN;
    }
    if (graBits == MQB_PADDLE_BOTH)
    {
      graBits = MQB_PADDLE_NONE; // PQ only knows a single up or down, so both together sends nothing
    }

    if (sendPQ && (tick % 2) == 0)
    {
      sendGraPaddleFrame(graBits);
    }

    // --- 5. What the web page shows as "Sending" ---
    if (sendMQB)
    {
      paddleCmdSent = mqbBits;
    }
    else if (sendPQ)
    {
      paddleCmdSent = graBits;
    }
    else
    {
      paddleCmdSent = MQB_PADDLE_NONE;
    }

    tick++;
    if (tick >= (MQB_PADDLE_REFRESH_MS / broadcastPaddlesRefresh)) // 100 / 10 = every 10th loop
    {
      tick = 0; // back to the 100 ms slot
    }

    vTaskDelay(pdMS_TO_TICKS(broadcastPaddlesRefresh));
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