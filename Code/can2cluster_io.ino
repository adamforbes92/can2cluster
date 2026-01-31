void basicInit() {
// basic initialisation - setup pins for IO & setup CAN for receiving...

// if ANY Serial request is made, begin Serial
#if enableDebug || serialDebugWifi || serialDebugEEP || serialDebugGPS || ChassisCANDebug || serialDebugPaddles || serialDebugIO
  Serial.begin(baudSerial);
  delay(500);
  DEBUG("CAN-BUS to Cluster Initialising...");
#endif

#if enableDebug
  DEBUG("Reading EEPROM...");
#endif
  readEEP();  // read EEPROM
#if enableDebug
  DEBUG("Read EEPROM!");
#endif

  ss.begin(baudGPS);  // begin GPS Module
#if detailedDebugGPS
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
  Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
  Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
#endif

#if enableDebug
  DEBUG("Setting up IO (pins & buttons)...");
#endif
  setupPins();     // begin IO
  setupButtons();  // setup buttons for interrupt
#if enableDebug
  DEBUG("Set up IO complete!");
#endif

#if enableDebug
  DEBUG("CAN Chip Initialising...");
#endif
  canInit();  // initialise the CAN chip
#if enableDebug
  DEBUG("CAN Chip Initialised!");
#endif
}

void setupPins() {
  // define pin modes for outputs
  pinMode(onboardLED, OUTPUT);  // use the built-in LED for displaying errors!

  pinMode(pinCoil, OUTPUT);  // for high-voltage RPM (can be turned on/off in WiFi so always enable regardless)
  pinMode(pinRPM, OUTPUT);   // for standard square wave RPM

  pinMode(pinSpeed, OUTPUT);    // for speed output
  pinMode(pinEML, OUTPUT);      // for engine management light output
  pinMode(pinEPC, OUTPUT);      // for electronic pedal control output
  pinMode(pinReverse, OUTPUT);  // for reverse MOSFET output (5A max!)

  //pinMode(pinPaddleUp, INPUT);                                                 // for DSG paddle up - pull to ground
  //pinMode(pinPaddleDown, INPUT);                                               // for DSG paddles down - pull to ground
  attachInterrupt(digitalPinToInterrupt(pinHallSensor), incomingHz, FALLING);          //setup interrupt to toggle pin on change
  attachInterrupt(digitalPinToInterrupt(pinMotorInput), incomingMotorSpeed, FALLING);  //setup interrupt to toggle pin on change
}

void setupButtons() {
  //setup buttons / inputs
  btnPadUp.setMenuCount(0);
  btnPadDown.setMenuCount(0);

  //InterruptButton::m_RTOSservicerStackDepth = 4096; // Use larger values for more memory intensive functions if using Asynchronous mode.
  btnPadUp.bind(Event_KeyPress, 0, &padUpFunc);
  btnPadDown.bind(Event_KeyPress, 0, &padDownFunc);

  btnPadUp.setMode(Mode_Asynchronous);
  btnPadDown.setMode(Mode_Asynchronous);
}

void setupTasks() {
  xTaskCreate(showState, "showState", 8000, NULL, 1, NULL);                              // for Serial feedback
  xTaskCreate(checkError, "checkError", 1000, NULL, 2, NULL);                            // for Serial feedback
  xTaskCreate(updateLabels, "updateLabels", 8000, NULL, 3, NULL);                        // for WiFi labels
  xTaskCreate(writeEEP, "writeEEP", 3000, NULL, 4, NULL);                                // to update EEPROM
  xTaskCreate(processOutputs, "processOutputs", 2000, NULL, 5, &handle_processOutputs);  // to update outputs
  xTaskCreate(broadcastSpeed, "broadcastSpeed", 2000, NULL, 6, NULL);
  xTaskCreate(broadcastGRA, "broadcastGRA", 4000, NULL, 6, NULL);

  xTaskCreate(parseGPS, "parseGPS", 2000, NULL, 7, NULL);
  xTaskCreate(parseSpeed, "parseSpeed", 2000, NULL, 9, NULL);
  xTaskCreate(parseRPM, "parseRPM", 2000, NULL, 8, NULL);
  xTaskCreate(parseShiftLights, "parseShiftLights", 1000, NULL, 9, &handle_parseShiftLights);
}

void showState(void *arg) {
  while (1) {
    stackshowState = uxTaskGetStackHighWaterMark(NULL);

#if enableDebug
    DEBUG("Basic Debug Info:");                                      // this means it has a clutch issue
    DEBUG("    Has CAN: %d", hasCAN);                                // this means it has a clutch issue
    DEBUG("    Has GPS: %d", hasGPS);                                // this means it also has a clutch issue
    DEBUG("    testRPM / canRPM: %d / %d", testRPM, vehicleRPMCAN);  // clutch fully disengaged
    DEBUG("    testSpeedo: %d", testSpeedo);                         // hit a speed limit...
    DEBUG("    Hall Speed: %d", hallSpeed);                          // incrememting value for checking the response to vars...
    DEBUG("    GPS Speed: %d", gpsSpeed);                            // incrememting value for checking the response to vars...
    DEBUG("    ECU Speed: %d", ecuSpeed);                            // incrememting value for checking the response to vars...
    DEBUG("    ABS Speed: %d", absSpeed);                            // incrememting value for checking the response to vars...
    DEBUG("    DSG Speed: %d", dsgSpeed);                            // incrememting value for checking the response to vars...
    DEBUG("");                                                       // incrememting value for checking the response to vars...
#endif

#if detailedDebugStack
    DEBUG("Stack Sizes:");
    DEBUG("    stackshowState: %d", stackshowState);    // incrememting value for checking the response to vars...
    DEBUG("    stackcheckError: %d", stackcheckError);  // incrememting value for checking the response to vars...

    DEBUG("    stackupdateLabels: %d", stackupdateLabels);      // incrememting value for checking the response to vars...
    DEBUG("    stackwriteEEP: %d", stackwriteEEP);              // incrememting value for checking the response to vars...
    DEBUG("    stackprocessOutputs: %d", stackprocessOutputs);  // incrememting value for checking the response to vars...
    DEBUG("    stackbroadcastSpeed: %d", stackbroadcastSpeed);  // incrememting value for checking the response to vars...
    DEBUG("    stackbroadcastGRA: %d", stackbroadcastGRA);      // incrememting value for checking the response to vars...
    DEBUG("    stackparseGPS: %d", stackparseGPS);              // incrememting value for checking the response to vars...

    DEBUG("    stackparseSpeed: %d", stackparseSpeed);              // incrememting value for checking the response to vars...
    DEBUG("    stackparseRPM: %d", stackparseRPM);                  // incrememting value for checking the response to vars...
    DEBUG("    stackparseShiftLights: %d", stackparseShiftLights);  // incrememting value for checking the response to vars...
#endif

#if ChassisCANDebug
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

#if detailedDebugGPS
    DEBUG("Detailed GPS Info:");                          // this means it has a clutch issue
    DEBUG("    Satellites: %d", gps.satellites.value());  // incrememting value for checking the response to vars...
    DEBUG("    HDOP: %d", gps.hdop.hdop());               // incrememting value for checking the response to vars...
    DEBUG("    HDOP: %d", gps.hdop.hdop());               // incrememting value for checking the response to vars...
    printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
    printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
    DEBUG("    GPS Speed: %d", gpsSpeed);  // incrememting value for checking the response to vars...
    DEBUG("");                             // incrememting value for checking the response to vars...
#endif

#if detailedDebugIO
    DEBUG("Detailed IO Info:");                  // this means it has a clutch issue
    DEBUG("    Test EML: %d", testEML);          // this means it has a clutch issue
    DEBUG("    Test EPC: %d", testEPC);          // this means it also has a clutch issue
    DEBUG("    Test Reverse: %d", testReverse);  // clutch fully disengaged
#endif

    vTaskDelay(serialMonitorRefresh / portTICK_PERIOD_MS);
  }
}

void needleSweep() {
  DEBUG("Starting needle sweep...");

  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(sweepSpeed);

  // ramp up
  for (int i = 0; i < maxRPM; i++) {
    setFrequencySpeed(i * (stepSpeed / 10));
    setFrequencyRPM(i * (stepRPM / 10));
    delay(sweepSpeed);
  }
  delay(sweepSpeed);

  // ramp down
  for (int i = maxRPM; i > 0; i--) {  // set at >0 to stop the needle 'bouncing' when it returns to zero
    setFrequencySpeed(i * (stepSpeed / 10));
    setFrequencyRPM(i * (stepRPM / 10));
    delay(sweepSpeed);
  }

  delay(sweepSpeed);  // hold at max RPM (to stop immediate return)

  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(sweepSpeed);

  DEBUG("Finished needle sweep...");
}

void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML, bool boolRPM, bool boolSpeed) {
  for (int i = 0; i < flashes; i++) {
    if (boolEPC) {
      delay(duration);
      digitalWrite(pinEPC, HIGH);
      delay(duration);
      digitalWrite(pinEPC, LOW);
    }
    if (boolEML) {
      delay(duration);
      digitalWrite(pinEML, HIGH);
      delay(duration);
      digitalWrite(pinEML, LOW);
    }
    if (boolRPM) {
      delay(duration);
      digitalWrite(pinRPM, HIGH);
      delay(duration);
      digitalWrite(pinRPM, LOW);
    }
    if (boolSpeed) {
      delay(duration);
      digitalWrite(pinSpeed, HIGH);
      delay(duration);
      digitalWrite(pinSpeed, LOW);
    }
  }
}

void diagTest() {
  vehicleRPM = +1000;
  vehicleSpeed = +10;

  if (vehicleRPM > clusterRPMLimit) {
    vehicleRPM = 1000;
    frequencyRPM = 1;
  }
  if (vehicleSpeed > maxSpeed) {
    vehicleSpeed = 1;
    frequencySpeed = 1;
  }

  vehicleReverse = !vehicleReverse;
  digitalWrite(pinReverse, vehicleReverse);

  blinkLED(1000, 1, 1, 1, 0, 0);
}

void checkError(void *arg) {
  while (1) {
    stackcheckError = uxTaskGetStackHighWaterMark(NULL);

    hasError ? triggerLED = !triggerLED : triggerLED = false;
    triggerLED ? digitalWrite(onboardLED, HIGH) : digitalWrite(onboardLED, LOW);

    boolPadUpWiFi = false;
    boolPadDownWiFi = false;

    vehicleRPMHall = 0;

    vTaskDelay(checkErrorRefresh * 2 / portTICK_PERIOD_MS);
  }
}

// adjust output frequency
void setFrequencyRPM(long frequencyHz) {
  if (frequencyHz != 0) {
    timerAlarmDisable(timer0);
    timerAlarmWrite(timer0, 1000000l / frequencyHz, true);
    timerAlarmEnable(timer0);
  } else {
    timerAlarmDisable(timer0);
  }
}

// adjust output frequency
void setFrequencySpeed(long frequencyHz) {
  if (frequencyHz != 0) {
    timerAlarmDisable(timer1);
    timerAlarmWrite(timer1, 1000000l / frequencyHz, true);
    timerAlarmEnable(timer1);
  } else {
    timerAlarmDisable(timer1);
  }
}

void incomingHz() {                                               // Interrupt 0 service routine
  static unsigned long previousMicros = micros();                 // remember variable, initialize first time
  unsigned long presentMicros = micros();                         // read microseconds
  unsigned long revolutionTime = presentMicros - previousMicros;  // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL) return;                            // avoid divide by 0, also debounce, speed can't be over 60,000 was 1000UL
  dutyCycleIncoming = (60000000UL / revolutionTime) / 60;         // calculate
  previousMicros = presentMicros;
  lastPulse = millis();
}

void incomingMotorSpeed() {                                       // Interrupt 0 service routine
  static unsigned long previousMicros = micros();                 // remember variable, initialize first time
  unsigned long presentMicros = micros();                         // read microseconds
  unsigned long revolutionTime = presentMicros - previousMicros;  // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL) return;                            // avoid divide by 0, also debounce, speed can't be over 60,000 was 1000UL
  dutyCycleMotor = (60000000UL / revolutionTime) / 60;            // calculate
  previousMicros = presentMicros;

  lastPulseRPM = millis();
}

void parseSpeed(void *arg) {
  while (1) {
    stackparseSpeed = uxTaskGetStackHighWaterMark(NULL);

    if (testSpeedo) {
      vehicleSpeed = tempSpeed;
      if (speedUnits == 1) {
        vehicleSpeed = int((vehicleSpeed * mphFactor) / 1000000);  //621371
      }
    }

    if (!testSpeedo) {
      vehicleSpeed = 0;

      hallSpeed = map(dutyCycleIncoming, 0, maxFreqHall, 0, maxSpeed);  // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
      parseDSG();

      if (useHall) {
        if (hallSpeed > 0) {
          vehicleSpeed = (byte)(hallSpeed >= 255 ? 0 : hallSpeed);
        }
      }
      if (useDSG) {
        vehicleSpeed = int(dsgSpeed);
      }
      if (useGPS) {
        vehicleSpeed = int(gpsSpeed);
      }
      if (useABS) {
        vehicleSpeed = int(absSpeed);
      }
      if (useECU) {
        vehicleSpeed = int(ecuSpeed);
      }
      if (speedUnits == 1) {
        vehicleSpeed = int((vehicleSpeed * mphFactor) / 1000000);  //621371
      }
    }
    vTaskDelay(rpmPause / portTICK_PERIOD_MS);
  }
}

void parseRPM(void *arg) {
  while (1) {
    stackparseRPM = uxTaskGetStackHighWaterMark(NULL);

    vehicleRPM = 0;
    vehicleRPMHall = map(dutyCycleMotor, 0, maxFreqRPM, 0, maxRPM);  // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
    if (vehicleRPMHall > 0) {
      vehicleRPM = (byte)(vehicleRPMHall >= 255 ? 0 : vehicleRPMHall);
    }

    testRPM ? vehicleRPM = tempRPM : vehicleRPM = vehicleRPMCAN;

    vTaskDelay(rpmPause / portTICK_PERIOD_MS);
  }
}

void parseShiftLights(void *arg) {
  while (1) {
    stackparseShiftLights = uxTaskGetStackHighWaterMark(NULL);

    // check to see what the current RPM is, if it's over the limit, trigger the EPC or EML light as a warning!
    if (useEPCShiftLight || useEMLShiftLight) {
      if (vehicleRPM > shiftLimit) {
        blinkLED(shiftLightRate, shiftFlashes, useEPCShiftLight, useEMLShiftLight, 0, 0);  // args: flash rate, number of flashes, use EPC or use EML as light, RPM/Speed are set to 0, don't use them (kept in for self-test)
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void processOutputs(void *arg) {
  while (1) {
    stackprocessOutputs = uxTaskGetStackHighWaterMark(NULL);

    // set EML, EPC & Reverse outputs
    if (testEML || testPark) {
      digitalWrite(pinEML, HIGH);
    }
    if (!testEML && useEMLPark) {
      digitalWrite(pinEML, vehiclePark);
    }
    if (!testEML && !useEMLPark) {
      digitalWrite(pinEML, vehicleEML);
    }

    if (testEPC || testPark) {
      digitalWrite(pinEPC, HIGH);
    }
    if (!testEPC && useEPCPark) {
      digitalWrite(pinEPC, vehiclePark);
    }
    if (!testEPC && !useEPCPark) {
      digitalWrite(pinEPC, vehicleEML);
    }

    testReverse ? digitalWrite(pinReverse, HIGH) : digitalWrite(pinReverse, vehicleReverse);

    // calculate final frequency:
    frequencySpeed = map(vehicleSpeed, 0, maxSpeed, 0, maxSpeed);
    frequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRPM);

    // change the frequency of both RPM & Speed as per CAN information
    setFrequencyRPM(frequencyRPM);      // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
    setFrequencySpeed(frequencySpeed);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'  }

    vTaskDelay(rpmPause / portTICK_PERIOD_MS);
  }
}