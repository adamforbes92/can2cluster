void canInit() {
  chassisCAN.setRX(pinRX_CAN);
  chassisCAN.setTX(pinTX_CAN);
  chassisCAN.setBaudRate(500000);  // CAN Speed in Hz
  chassisCAN.onReceive(onBodyRX);
  chassisCAN.begin();
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
  lastCAN = millis();

  switch (frame.id) {
    case MOTOR1_ID:
      // frame[2] (byte 3) > motor speed low byte
      // frame[3] (byte 4) > motor speed high byte
      // frame[4] (byte 3) > khm speed?
      vehicleRPMCAN = ((frame.buf[3] << 8) | frame.buf[2]) * 0.25;  // conversion: 0.25*HEX
      break;

    case MOTOR2_ID:
      calcSpeed = (frame.buf[3] * 100 * 128) / 10000;
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
        vehiclePark = true;  // unused bool, but a good to have...
      } else {
        vehiclePark = false;  // unused bool, but a good to have...
      }
      break;

    case BRAKES3_ID:
      absSpeed = ((frame.buf[3] << 8) | frame.buf[2]) * 1.28;  // conversion: 0.25*HEX
      break;

    case mWaehlhebel_1_ID:
#if serialDebugDSG
      DEBUG_PRINTLN(frame.buf[7]);
#endif
      gear_raw = ((frame.buf[7] & 0b01110000) >> 4) - 1;  // 0b01110000) >> 4) - 1;
      lever_raw = (frame.buf[7] & 0b00000001);

      if (lever_raw) {
        gear = gear_raw;
        switch (gear) {
          case 3:  // reverse
            vehicleReverse = true;
            break;
          default:
            vehicleReverse = false;
            break;
        }

        if (gear == 0xFF) {
          gear = 1;
        }
      }
      break;

    case gearLever_ID:
#if serialDebugDSG
      DEBUG_PRINTLN(frame.buf[0]);
#endif
      lever = (frame.buf[0] & 0b11110000) >> 4;
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

#if serialDebug
  Serial.println("From CAN:");
  Serial.print("vehicleRPM: ");
  Serial.println(vehicleRPM);

  Serial.print("vehicleSpeed: ");
  Serial.println(vehicleSpeed);

  Serial.print("Reverse: ");
  Serial.println(vehicleReverse);

  Serial.print("vehicleEML: ");
  Serial.println(vehicleEML);

  Serial.print("vehicleEPC: ");
  Serial.print(vehicleEPC);
#endif
}

/*
GRA_Neu   0x38A   CHK_GRA_Neu               1   0..7   8   gültiger Wert   0   255   0 .. 255      0   1      
GRA_Neu   0x38A   GRA_Hauptschalter         2   0   1   gültiger Wert                     0 1   Gerastet Aus EIN
GRA_Neu   0x38A   Abbrechen                 2   1   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   KurzTip_down              2   2   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   KurzTip_up                2   3   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   LangTip_down              2   4   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   LangTip_up                2   5   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   Bedienteil_Fehler         2   6   1   gültiger Wert                     0 1   i. O. Fehler Bedienteil
GRA_Neu   0x38A   Codierinfo_SMLS           2   7   1   gültiger Wert                     0 1   GRA codiert ACC codiert
GRA_Neu   0x38A   Tip_Setzen                3   0   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   Tip_Wiederaufnahme        3   1   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   Sendercodierung           3   2..3   2   gültiger Wert                     0 1 2 3   Bordnetzsteuergeraet Lenksaeulenmodul Motor SG nicht belegt
GRA_Neu   0x38A   BZ_GRA_Neu                3   4..7   4   gültiger Wert   0   15   0 .. 15      0   1      
GRA_Neu   0x38A   Tiptronic_Tip_Down        4   0   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   Tiptronic_Tip_Up          4   1   1   gültiger Wert                     0 1   Tippschalter nicht betaetigt Tippschalter betaetigt
GRA_Neu   0x38A   ACC_Zeitlueckenverstellung4   2..3   2   gültiger Wert                     0 1 2   Taste nicht betaetigt Dist -1 Dist +1
GRA_Neu   0x38A   Tiptronic_Limiter         4   4   1   gültiger Wert                     0 1   Limiter aus Limiter ein
GRA_Neu   0x38A   Typ_Hauptschalter         4   5   1   gültiger Wert   0   1   0 .. 1      0   1      
GRA_Neu   0x38A   void                      4   6                                             
GRA_Neu   0x38A   Tiptronic_Tip_Fehler      4   7   1   gültiger Wert                     0 1   i.O. Fehler erkannt

CHKSM: checksum
Bit addr. 0, bit num. 8, initial value 0
Valid range of values ​​0x00..0xFF

S_HAUPT: GRA / ADR - main switch
Bit addr. 8, bit num. 1, initial value 0
0 switched off, 1 switched on
RCOS message: mrmGRA

T_AUS: GRA / ADR - Tip switch "Off"
Bit addr. 9, bit num. 1, initial value 0
0 tip switch not activated, 1 tip switch activated
RCOS message: mrmGRA

T_VER: GRA / ADR - Tip switch "Decelerate"
Bit addr. 10, bit num. 1, initial value 0
0 tip switch not activated, 1 tip switch activated
RCOS message: mrmGRA

T_BES: GRA / ADR - Tip switch "Accelerate"
Bit addr. 11, bit num. 1, initial value 0
0 tip switch not activated, 1 tip switch activated
RCOS message: mrmGRA

ZU_VER: GRA / ADR delay; is not processed
Bit addr. 12, bit num. 1, initial value 0
0 Don't accelerate, 1 accelerate
RCOS message: mrmGRA

ZU_BES: GRA / ADR accelerate; is not processed
Bit addr. 13, bit num. 1, initial value 0
0 Don't delay, 1 delay
RCOS message: mrmGRA

F_BTL: GRA / ADR - keypad error
Bit addr. 14, bit num. 1, initial value 0
0 OK, 1 control lever error
RCOS message: mrmGRA

T_SET: GRA / ADR - Tip switch "Set"
Bit addr. 16, bit num. 1, initial value 0
0 tip switch not activated, 1 tip switch activated
RCOS message: mrmGRA

T_WA: GRA / ADR - Tip switch "Resume"
Bit addr. 17, bit num. 1, initial value 0
0 tip switch not activated, 1 tip switch activated
RCOS message: mrmGRA

COD_SND: Sender coding
Bit addr. 18, bit num. 2, initial value 0
00 On-board power supply control unit
01 steering column module
10 engine SG
11 not used
RCOS message: mrmGRA

Z_Count: message counter
Bit addr. 20, bit num. 4, initial value 0 Valid range of values ​​0x0..0xF

T_TDN: tip-down; is not processed
Bit addr. 24, bit num. 1, initial value 0
0 tip switch not actuated, 1 tip down

T_TUP: Tip-Up; is not processed
Bit addr. 25, bit num. 1, initial value 0
0 tip switch not actuated, 1 tip up

T_DST: ADR - Tip switch distance request; is not processed
Bit addr. 26, bit num. 2, initial value 0
00 Key not pressed
01 Nobody wants distance
10 Desired distance greater
11 not used

ZU_LIM: Limiter on; is not processed
Bit addr. 28, bit num. 1, initial value 0
0 tip switch not actuated, 1 tip up

F_BTLT: Tiptronic control unit error; is not processed
Bit addr. 31, bit num. 1, initial value 0
0 tip switch not actuated, 1 tip up*/

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

void broadcastGRA() {
  CAN_message_t broadcastGRA;
  broadcastGRA.id = GRA_ID;
  broadcastGRA.len = 4;
  //broadcastGRA.buf[0] = GRA_crc; - calculated soon...
  broadcastGRA.buf[1] = 0x00;         // always zero
  broadcastGRA.buf[2] = GRA_counter;  // counter (0x00 > 0xF0)
  if (boolPadUp) {
#if serialDebugPaddles
    DEBUG_PRINTLN("Paddle up");
#endif
    broadcastGRA.buf[3] = 0x02;
    boolPadUp = false;
  }

  if (boolPadDown) {
#if serialDebugPaddles
    DEBUG_PRINTLN("Paddle down");
#endif
    broadcastGRA.buf[3] = 0x01;
    boolPadDown = false;
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

void broadcastSpeed() {
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
}