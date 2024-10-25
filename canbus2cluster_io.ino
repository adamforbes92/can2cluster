void basicInit() {
// basic initialisation - setup pins for IO & setup CAN for receiving...
#if stateDebug
  Serial.begin(baudSerial);
  Serial.println(F("CAN-BUS to Cluster Initialising..."));
#endif
#if stateDebug
  Serial.println(F("Setting up pins for IO..."));
#endif
  setupPins();  // initialise the CAN chip
#if stateDebug
  Serial.println(F("Setup pins complete!"));
#endif

#if stateDebug
  Serial.println(F("CAN Chip enabling..."));
#endif
  canInit();  // initialise the CAN chip
#if stateDebug
  Serial.println(F("CAN Chip enabled!"));
#endif
}

void setupPins() {
  // define pin modes for outputs
  pinMode(pinRPM, OUTPUT);
  pinMode(pinSpeed, OUTPUT);
  pinMode(pinEML, OUTPUT);
  pinMode(pinEPC, OUTPUT);

#if hasCoilOutput
  pinMode(pinCoil, OUTPUT);
#endif
}

void needleSweep() {
#if stateDebug
  Serial.println("Carrying out needle sweep!");
#endif

  while (frequencyRPM < maxRRM) {
    setFrequencyRPM(frequencyRPM);
    setFrequencySpeed(frequencySpeed);

    // scaling?...
    frequencyRPM += 5;
    frequencySpeed += 1;
    delay(needleSweepDelay);
  }

#if stateDebug
  Serial.println("Finished needle sweep!");
#endif
}

void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML) {
  if (boolEPC) {
    for (int i = 0; i < flashes; i++) {
      delay(duration);
      digitalWrite(pinEPC, HIGH);
      delay(duration);
      digitalWrite(pinEPC, LOW);
    }
  }

  if (boolEML) {
    for (int i = 0; i < flashes; i++) {
      delay(duration);
      digitalWrite(pinEML, HIGH);
      delay(duration);
      digitalWrite(pinEML, LOW);
    }
  }
}