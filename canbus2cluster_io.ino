void basicInit() {
// basic initialisation - setup pins for IO & setup CAN for receiving...
#if stateDebug
  Serial.begin(baudSerial);
  Serial.println(F("CAN-BUS to Cluster Initialising..."));
  Serial.println(F("Setting up pins..."));
#endif

  if (speedType == 2) {
    ss.begin(baudGPS);
#if stateDebug
    Serial.println(TinyGPSPlus::libraryVersion());
    Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
    Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
    Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
#endif
  }
  setupPins();  // initialise the CAN chip
  setupButtons();

#if stateDebug
  Serial.println(F("Setup pins complete!"));
#endif

#if stateDebug
  Serial.println(F("CAN Chip Initialising..."));
#endif
  canInit();  // initialise the CAN chip
#if stateDebug
  Serial.println(F("CAN Chip Initialised!"));
#endif
}

void setupPins() {
  // define pin modes for outputs
  pinMode(onboardLED, OUTPUT);  // use the built-in LED for displaying errors!

  pinMode(pinRPM, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  pinMode(pinEML, OUTPUT);
  pinMode(pinEPC, OUTPUT);
  pinMode(pinReverse, OUTPUT);

#if hasCoilOutput
  pinMode(pinCoil, OUTPUT);
#endif

  // reset buttons if testLED is used (can be removed if 'testLED' is not used but keeping here for solidness)
  pinMode(pinPaddleUp, INPUT_PULLUP);
  pinMode(pinPaddleDown, INPUT_PULLUP);
  pinMode(pinSpare1, INPUT_PULLUP);
  pinMode(pinSpare2, INPUT_PULLUP);
}

void setupButtons() {
  //setup buttons / inputs
  btnPadUp.attachSingleClick(padUpFunc);      // call intSingle on a single click (single wipe)
  btnPadDown.attachSingleClick(padDownFunc);  // call intSingle on a single click (single wipe)

  btnSpare1.attachSingleClick(spare1Func);  // call intSingle on a single click (single wipe)
  btnSpare2.attachSingleClick(spare2Func);  // call intSingle on a single click (single wipe)
}

void needleSweep() {
  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(needleSweepDelay);

#if stateDebug
  Serial.println(F("Starting needle sweep..."));
#endif

  while ((frequencyRPM != maxRPM) && (frequencySpeed != maxSpeed)) {
    setFrequencyRPM(frequencyRPM);
    setFrequencySpeed(frequencySpeed);

    // scaling?...
    frequencyRPM += stepRPM;
    frequencySpeed += stepSpeed;
    delay(needleSweepDelay);  // increase or decrease the needle sweep speed in _defs
  }

  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(needleSweepDelay * 20);

#if stateDebug
  Serial.println(F("Finished needle sweep!"));
#endif
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
  vehicleRPM += 1000;
  vehicleSpeed += 10;

  if (vehicleRPM > clusterRPMLimit) {
    vehicleRPM = 0;
    frequencyRPM = 0;
  }
  if (vehicleSpeed > clusterSpeedLimit) {
    vehicleSpeed = 0;
    frequencySpeed = 0;
  }

  vehicleReverse = !vehicleReverse;
  //digitalWrite(pinReverse, vehicleReverse);

  blinkLED(1000, 1, 1, 1, 0, 0);
}