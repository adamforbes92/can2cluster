void canInit() {
  chassisCAN.setRX(pinRX_CAN);
  chassisCAN.setTX(pinTX_CAN);
  chassisCAN.setBaudRate(baudCAN);  // CAN Speed in Hz
  chassisCAN.onReceive(onBodyRX);
  chassisCAN.begin();
  // set filters up for focusing on only MOT1 / MOT 2?
}

void onBodyRX(const CAN_message_t &frame) {
#if ChassisCANDebug  // print incoming CAN messages
  Serial.print("Length Recv: ");
  Serial.print(frame.len);
  Serial.print(" CAN ID: ");
  Serial.print(frame.id, HEX);
  Serial.print(" Buffer: ");
  for (uint8_t i = 0; i < frame.len; i++) {
    Serial.print(frame.buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif
  lastCAN = millis();

  switch (frame.id) {
    case MOTOR1_ID:
      // frame[2] (byte 3) > motor speed low byte
      // frame[3] (byte 4) > motor speed high byte
      // frame[4] (byte 3) > khm speed?
      vehicleRPMCAN = ((frame.buf[3] << 8) | frame.buf[2]) * 0.25;  // conversion: 0.25*HEX
      break;

    case MOTOR2_ID:
      ecuSpeed = (frame.buf[3] * 100 * 128) / 10000;
      break;

    case MOTOR5_ID:
      // set EML & EPC based on the bit read (LSB, so backwards)
      vehicleEML = bitRead(frame.buf[1], 5);
      vehicleEPC = bitRead(frame.buf[1], 6);
      break;

    case MOTOR6_ID:
      if (frame.buf[0] == 0x73 || frame.buf[0] == 0x72) {
        //vehicleReverse = true;
      } else {
        //vehicleReverse = false;
      }
      if (frame.buf[0] == 0x83 || frame.buf[0] == 0x82) {
        //vehiclePark = true;  // unused bool, but a good to have...
      } else {
        //vehiclePark = false;  // unused bool, but a good to have...
      }
      break;

    case BRAKES3_ID:
      if (calcSpeed == 0) {
        absSpeed = ((frame.buf[0] << 8) | frame.buf[1]);  // conversion: 0.25*HEX
      }
      break;

    case mWaehlhebel_1_ID:
      gear_raw = ((frame.buf[7] & 0b01110000) >> 4) - 1;  // 0b01110000) >> 4) - 1;
      lever_raw = (frame.buf[7] & 0b00000001);

      if (lever_raw) {
        gear = gear_raw;
        switch (gear) {
          case 3:  // reverse
            //vehicleReverse = true;
            break;
          default:
            //vehicleReverse = false;
            break;
        }

        if (gear == 0xFF) {
          gear = 1;
        }
      }
      break;

    case gearLever_ID:
      lever = (frame.buf[0] & 0b11110000) >> 4;
      switch (lever) {
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
      vehicleRPM = ((frame.buf[0] << 8) | frame.buf[1]);  // conversion: 0.25*HEX // this is RPM
      break;

    case emeraldECU2_ID:
      vehicleSpeed = ((frame.buf[2] << 8) | frame.buf[3]) * (2.25 / 256);  // conversion: 0.25*HEX // this is RPM
      break;

    case fordECU1_ID:
      vehicleRPMCAN = frame.buf[1] & 0x00FF;
      vehicleRPMCAN |= (frame.buf[0] << 8) & 0x7F00;
      break;

    case fordECU2_ID:
      vehicleOilPressure = (frame.buf[5] >> 4) & 0x01;
      vehicleBattLight = (frame.buf[4] >> 1) & 0x01;
      vehicleEML = (frame.buf[4] >> 6) & 0x03;
      vehicleCoolantTemp = (frame.buf[0] & 0xFF) - 40;
      break;

    default:
      // do nothing...
      break;
  }
}

void sendPaddleUpFrame() {
  CAN_message_t paddlesUp;  //0x7C0
  paddlesUp.id = GRA_ID;
  paddlesUp.len = 4;
  paddlesUp.buf[0] = 0x0E;             // was 0xB7 chksum = byte 2 XOR byte 3 XOR byte 4
  paddlesUp.buf[2] = 0x0C;             // was 0x34
  paddlesUp.buf[3] = 0x02;             //
  bitSet(paddlesUp.buf[3], 1);         // set high (trigger)
  if (!chassisCAN.write(paddlesUp)) {  // write CAN frame from the body to the Haldex
  }
}

void sendPaddleDownFrame() {
  CAN_message_t paddlesDown;  //0x7C0
  paddlesDown.id = GRA_ID;
  paddlesDown.len = 4;
  paddlesDown.buf[0] = 0x0D;  // chksum = byte 2 XOR byte 3 XOR byte
  paddlesDown.buf[2] = 0x0C;
  paddlesDown.buf[3] = 0x01;
  bitSet(paddlesDown.buf[3], 1);         // set high (trigger)
  if (!chassisCAN.write(paddlesDown)) {  // write CAN frame from the body to the Haldex
  }
}

void broadcastGRA(void *arg) {
  while (1) {
    stackbroadcastGRA = uxTaskGetStackHighWaterMark(NULL);

    if (useDSG) {
      CAN_message_t broadcastGRA;
      broadcastGRA.id = GRA_ID;
      broadcastGRA.len = 4;
      //broadcastGRA.buf[0] = GRA_crc; - calculated soon...
      broadcastGRA.buf[1] = 0x00;         // always zero
      broadcastGRA.buf[2] = GRA_counter;  // counter (0x00 > 0xF0)
      if (boolPadUp) {
        broadcastGRA.buf[3] = 0x02;
        boolPadUp = false;
        boolPadUpWiFi = true;
      }

      if (boolPadDown) {
        broadcastGRA.buf[3] = 0x01;
        boolPadDown = false;
        boolPadDownWiFi = true;
      }

      GRA_crc = 0;
      for (uint8_t i = 2; i < 5; i++) {
        GRA_crc ^= broadcastGRA.buf[i];  // xor byte 2, 3, 4
      }
      broadcastGRA.buf[0] = GRA_crc;

      if (!chassisCAN.write(broadcastGRA)) {  // write CAN frame from the body to the Haldex
      }

      GRA_counter = GRA_counter + 16;
      if (GRA_counter > 0xF0) {
        GRA_counter = 0;
      }
    }
    vTaskDelay(speedBroadcast / portTICK_PERIOD_MS);
  }
}

void broadcastSpeed(void *arg) {
  while (1) {
    stackbroadcastSpeed = uxTaskGetStackHighWaterMark(NULL);

    // todo
    /*  CAN_message_t broadcastSpeed;  //0x7C0
  broadcastSpeed.id = BRAKES1_ID;
  broadcastSpeed.len = 8;
  broadcastSpeed.buf[0] = vehicleSpeed;  //
  broadcastSpeed.buf[2] = vehicleSpeed;
  broadcastSpeed.buf[3] = 0x00;
    if (!chassisCAN.write(broadcastSpeed)) {  // write CAN frame from the body to the Haldex
  }
  */
    vTaskDelay(speedBroadcast / portTICK_PERIOD_MS);
  }
}