void basicInit() {
// basic initialisation - setup pins for IO & setup CAN for receiving...
#if stateDebug
  Serial.begin(baudSerial);
  Serial.println(F("CAN-BUS to Cluster Initialising..."));
  Serial.println(F("Setting up pins..."));
#endif

  if (hasGPSSpeed) {
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
  pinMode(LED_BUILTIN, OUTPUT); // use the built-in LED for displaying errors!

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
  frequencyRPM = 1;
  frequencySpeed = 1;
#if stateDebug
  Serial.println(F("Starting needle sweep..."));
#endif

  while (frequencyRPM < maxRRM) {
    setFrequencyRPM(frequencyRPM);
    setFrequencySpeed(frequencySpeed);

    // scaling?...
    frequencyRPM += 1;
    frequencySpeed += 1.8;
    delay(needleSweepDelay);  // increase or decrease the needle sweep speed in _defs
  }
  delay(needleSweepDelay * 100);

  frequencyRPM = 1;
  frequencySpeed = 1;
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
  //i = 0;
  i = i + 1000;
  if (i > clusterRPMLimit) {
    i = 0;
  }
  vehicleReverse = !vehicleReverse;

  finalFrequencySpeed = map(i, 0, clusterSpeedLimit, 0, maxSpeed);
  finalFrequencyRPM = map(i, 0, clusterRPMLimit, 0, maxRRM);
#if stateDebug
  Serial.print(F("Freq RPM: "));
  Serial.println(finalFrequencyRPM);
#endif

  // change the frequency of both RPM & Speed as per CAN information
  setFrequencySpeed(finalFrequencySpeed + 1);
  setFrequencyRPM(finalFrequencyRPM + 1);

  digitalWrite(pinReverse, vehicleReverse);

  blinkLED(100, 1, 1, 1, 1, 1);
}