void canInit() {
  chassisCAN.setRX(pinRX_CAN);
  chassisCAN.setTX(pinTX_CAN);
  chassisCAN.begin();
  chassisCAN.setBaudRate(500000);  // CAN Speed in Hz
  chassisCAN.onReceive(onBodyRX);
  // set filters up for focusing on only MOT1 / MOT 2?
}

void onBodyRX(const CAN_message_t& frame) {
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

  switch (frame.id) {
    case MOTOR1_ID:
      // frame[2] (byte 3) > motor speed low byte
      // frame[3] (byte 4) > motor speed high byte
      vehicleRPM = ((frame.buf[3] << 8) | frame.buf[2]) * 0.25;  // conversion: 0.25*HEX
      break;
    case MOTOR2_ID:
      calcSpeed = (frame.buf[3] * 100 * 128) / 10000;
      vehicleSpeed = (byte)(calcSpeed >= 255 ? 0 : calcSpeed);
      break;
    case MOTOR5_ID:
      // frame[1] > free, bit 0
      // frame[1] > vorgl lampeu, bit 1
      // frame[1] > EPC lamp, bit 2
      // frame[1] > ODB2 (EML?) lamp, bit 3
      // frame[1] > cat lamp, bit 4

      // set EML & EPC based on the bit read (LSB, so backwards)
      vehicleEML = bitRead(frame.buf[1], 5);
      vehicleEPC = bitRead(frame.buf[1], 6);
      break;
    case MOTOR6_ID:
      if (frame.buf[0] == 0x73 || frame.buf[0] == 0x72) {
        vehicleReverse = true;
        digitalWrite(pinReverse, HIGH);
      } else {
        vehicleReverse = false;
        digitalWrite(pinReverse, LOW);
      }
      if (frame.buf[0] == 0x83 || frame.buf[0] == 0x82) {
        vehiclePark = true;
      } else {
        vehiclePark = false;
      }
      break;
    default:
      // do nothing...
      break;
  }
#if stateDebug
  Serial.println();
  Serial.print("vehicleRPM: ");
  Serial.println(vehicleRPM);

  Serial.print("vehicleSpeed: ");
  Serial.println(vehicleSpeed);

  Serial.print("vehicleEML: ");
  Serial.println(vehicleEML);

  Serial.print("vehicleEPC: ");
  Serial.print(vehicleEPC);
#endif
}

void sendPaddleUpFrame() {
  //canMsg1.can_id = 0x38A;
  //canMsg1.can_dlc = 4;
  //canMsg1.data[0] = 0xB7;
  //    bitSet(mShift[3], 1);
    //bitClear(mShift[3], 1);

  //canMsg1.data[2] = 0x34;
  //canMsg1.data[3] = 0x02;
}

void sendPaddleDownFrame() {
  //canMsg2.can_id = 0x38A;
  //canMsg2.can_dlc = 4;
  //canMsg2.data[0] = 0xB4;
  //canMsg2.data[1] = 0x81;
  //canMsg2.data[2] = 0x34;
  //canMsg2.data[3] = 0x01;
}