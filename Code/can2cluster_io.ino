void basicInit() {
// basic initialisation - setup pins for IO & setup CAN for receiving...

// if ANY Serial request is made, begin Serial
#if serialDebug || serialDebugWifi || serialDebugEEP || serialDebugGPS || ChassisCANDebug || serialDebugPaddles || serialDebugIO
  Serial.begin(baudSerial);
  delay(500);
  DEBUG("CAN-BUS to Cluster Initialising...");
#endif

#if serialDebug
  DEBUG("Reading EEPROM...");
#endif
  readEEP();  // read EEPROM
#if serialDebug
  DEBUG("Read EEPROM!");
#endif

  ss.begin(baudGPS);  // begin GPS Module
#if serialDebugGPS
  DEBUG(TinyGPSPlus::libraryVersion());
  DEBUG("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum");
  DEBUG("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  DEBUG("----------------------------------------------------------------------------------------------------------------------------------------");
#endif

#if serialDebug
  DEBUG("Setting up IO (pins & buttons)...");
#endif
  setupPins();     // begin IO
  setupButtons();  // setup buttons for interrupt
#if serialDebug
  DEBUG("Setup IO Complete!");
#endif

#if serialDebug
  DEBUG("CAN Chip Initialising...");
#endif
  canInit();  // initialise the CAN chip
#if serialDebug
  DEBUG("CAN Chip Initialised!");
#endif
}

void setupPins() {
  // define pin modes for outputs
  pinMode(onboardLED, OUTPUT);  // use the built-in LED for displaying errors!

  pinMode(pinSpeed, OUTPUT);    // for speed output
  pinMode(pinEML, OUTPUT);      // for engine management light output
  pinMode(pinEPC, OUTPUT);      // for electronic pedal control output
  pinMode(pinReverse, OUTPUT);  // for reverse MOSFET output (5A max!)

  pinMode(pinCoil, OUTPUT);  // for high-voltage RPM (can be turned on/off in WiFi so always enable regardless)
  pinMode(pinRPM, OUTPUT);   // for standard square wave RPM

  attachInterrupt(digitalPinToInterrupt(pinHallSensor), incomingHz, FALLING);  //setup interrupt to toggle pin on change
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
  xTaskCreate(showState, "showState", 8000, NULL, 1, NULL);
  xTaskCreate(updateLabels, "updateLabels", 8000, NULL, 3, NULL);
  xTaskCreate(writeEEP, "writeEEP", 2000, NULL, 4, NULL);

  xTaskCreate(broadcastGRA, "broadcastGRA", 4000, NULL, 5, NULL);
  xTaskCreate(broadcastSpeed, "broadcastSpeed", 4000, NULL, 6, NULL);

  xTaskCreate(parseGPS, "parseGPS", 6000, NULL, 7, NULL);
  xTaskCreate(parseDSG, "parseDSG", 6000, NULL, 8, NULL);

  xTaskCreate(updateSpeed, "updateSpeed", 2000, NULL, 9, NULL);
  xTaskCreate(updateRPM, "updateRPM", 2000, NULL, 10, NULL);

  xTaskCreate(checkError, "checkError", 2000, NULL, 11, NULL);
}

void showState(void *arg) {
  while (1) {
//stackshowHaldexState = uxTaskGetStackHighWaterMark(NULL);
#if detailedDebugStack
    stackShowState = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

#if detailedDebugStack
    DEBUG("Stack Sizes:");
    DEBUG("    stackShowState: %d", stackShowState);        // incrememting value for checking the response to vars...
    DEBUG("    stackUpdateLabels: %d", stackUpdateLabels);  // incrememting value for checking the response to vars...

    DEBUG("    stackWriteEEP: %d", stackWriteEEP);  // incrememting value for checking the response to vars...

    DEBUG("    stackbroadcastGRA: %d", stackbroadcastGRA);      // incrememting value for checking the response to vars...
    DEBUG("    stackbroadcastSpeed: %d", stackbroadcastSpeed);  // incrememting value for checking the response to vars...
    DEBUG("    stackparseGPS: %d", stackparseGPS);              // incrememting value for checking the response to vars...
    DEBUG("    stackparseDSG: %d", stackparseDSG);              // incrememting value for checking the response to vars...

    DEBUG("    stackupdateSpeed: %d", stackupdateSpeed);  // incrememting value for checking the response to vars...
    DEBUG("    stackupdateRPM: %d", stackupdateRPM);      // incrememting value for checking the response to vars...
    DEBUG("    stackshiftLight: %d", stackshiftLight);    // incrememting value for checking the response to vars...

    DEBUG("    stackcheckError: %d", stackcheckError);  // incrememting value for checking the response to vars...
#endif

#if ChassisCANDebug
    DEBUG("From CAN:");
    DEBUG("  vehicleRPM: %d", vehicleRPM);
    DEBUG("  vehicleSpeed: %d", vehicleSpeed);
    DEBUG("  vehicleReverse: %d", vehicleReverse);
    DEBUG("  vehicleEML: %d", vehicleEML);
    DEBUG("  vehicleEPC: %d", vehicleEPC);
#endif

#if serialDebugGPS
    DEBUG("From GPS:");
    DEBUG("  Satellites: %d", gps.satellites.value());
    DEBUG("  gpsSpeed: %d", gpsSpeed);
#endif

#if serialDebugIO
    DEBUG("Speeds:");
    DEBUG("  hallSpeed: %d", dutyCycleIncoming);
    DEBUG("  ecuSpeed: %d", calcSpeed);
    DEBUG("  dsgSpeed: %d", dsgSpeed);
    DEBUG("  gpsSpeed: %d", gpsSpeed);
    DEBUG("  absSpeed: %d", absSpeed);
#endif

    vTaskDelay(serialMonitorRefresh / portTICK_PERIOD_MS);
  }
}

void updateSpeed(void *args) {
  while (1) {
#if detailedDebugStack
    stackupdateSpeed = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

    if (!tempNeedleSweep) {  // only here if tested in WiFi
      if (testSpeedo) {
        vehicleSpeed = tempSpeed;
      } else {
        if (useHall) {
          vehicleSpeed = hallSpeed;
        }
        if (useECU) {
          vehicleSpeed = (byte)(calcSpeed >= 255 ? 0 : calcSpeed);
        }
        if (useABS) {
          vehicleSpeed = int(absSpeed);
        }
        if (useDSG) {
          vehicleSpeed = int(dsgSpeed);
        }
        if (useGPS) {
          vehicleSpeed = int(gpsSpeed);
        }
      }

      if (speedUnits == 1) {
        vehicleSpeed = int((vehicleSpeed * mphFactor) / 1000000);  //621371
      }

      // calculate final frequency:
      frequencySpeed = map(vehicleSpeed, 0, maxSpeed, 0, maxSpeed);
      setFrequencySpeed(frequencySpeed);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'  }
    }
    vTaskDelay(rpmPause / portTICK_PERIOD_MS);
  }
}

void updateRPM(void *args) {
  while (1) {
#if detailedDebugStack
    stackupdateRPM = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif
    if (!tempNeedleSweep) {  // only here if tested in WiFi
      if (testRPM) {         // set vehicleRPM is testing or not
        vehicleRPM = tempRPM;
      } else {
        vehicleRPM = vehicleRPMCAN;
      }

      frequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRPM);
      setFrequencyRPM(frequencyRPM);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
    }
    vTaskDelay(rpmPause / portTICK_PERIOD_MS);
  }
}

void needleSweep() {
  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(sweepSpeed);

  // ramp up
  for (int i = 0; i < maxRPM; i++) {
#if serialDebugIO
    DEBUG("stepSpeed: %d", int(i * (stepSpeed / 100)));
    DEBUG("stepRPM: %d", int(i * (stepRPM / 100)));
#endif
    setFrequencySpeed((i * stepSpeed) / 100);
    setFrequencyRPM((i * stepRPM) / 100);

    delay(sweepSpeed);
  }
  delay(sweepSpeed * 2);

  // ramp down
  for (int i = maxRPM; i > 0; i--) {  // set at >0 to stop the needle 'bouncing' when it returns to zero
#if serialDebugIO
    DEBUG("stepSpeed: %d", int(i * (stepSpeed / 100)));
    DEBUG("stepRPM: %d", int(i * (stepRPM / 100)));
#endif
    setFrequencySpeed((i * stepSpeed) / 100);
    setFrequencyRPM((i * stepRPM) / 100);
    delay(sweepSpeed);
  }

  setFrequencySpeed(10);
  delay(sweepSpeed * 2);  // hold at max RPM (to stop immediate return)

  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(sweepSpeed);
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

void checkError(void *args) {
  while (1) {
#if detailedDebugStack
    stackcheckError = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

    if (hasError) {
      triggerLED = !triggerLED;
    } else {
      triggerLED = false;
    }

    if (triggerLED) {
      digitalWrite(onboardLED, HIGH);  // turn internal LED on
    } else {
      digitalWrite(onboardLED, LOW);  // turn internal LED off
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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

  hallSpeed = map(dutyCycleIncoming, 0, maxFreqHall, 0, maxSpeed);  // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
}