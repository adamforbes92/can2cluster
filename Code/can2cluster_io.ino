void basicInit() {
// basic initialisation - setup pins for IO & setup CAN for receiving...

// if ANY Serial request is made, begin Serial
#if serialDebug || serialDebugWifi || serialDebugEEP || serialDebugGPS || ChassisCANDebug || serialDebugPaddles || serialDebugIO
  Serial.begin(baudSerial);
  delay(500);
  DEBUG_PRINTLN("CAN-BUS to Cluster Initialising...");
#endif

#if serialDebug
  DEBUG_PRINTLN("Reading EEPROM...");
#endif
  readEEP();  // read EEPROM
#if serialDebug
  DEBUG_PRINTLN("Read EEPROM!");
#endif

  ss.begin(baudGPS);  // begin GPS Module
#if serialDebugGPS
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum"));
  Serial.println(F("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail"));
  Serial.println(F("----------------------------------------------------------------------------------------------------------------------------------------"));
#endif

#if serialDebug
  Serial.println(F("Setting up IO (pins & buttons)..."));
#endif
  setupPins();     // begin IO
  setupButtons();  // setup buttons for interrupt
#if serialDebug
  DEBUG_PRINTLN("Setup IO Complete!");
#endif

#if serialDebug
  DEBUG_PRINTLN("CAN Chip Initialising...");
#endif
  canInit();  // initialise the CAN chip
#if serialDebug
  Serial.println(F("CAN Chip Initialised!"));
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

  //pinMode(pinPaddleUp, INPUT);                                                 // for DSG paddle up - pull to ground
  //pinMode(pinPaddleDown, INPUT);                                               // for DSG paddles down - pull to ground
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

void needleSweep() {
  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(sweepSpeed);

#if serialDebug
  Serial.println(F("Starting needle sweep..."));
#endif

  // ramp up
  for (int i = 0; i < maxRPM; i++) {
    setFrequencySpeed(i * stepSpeed);
    setFrequencyRPM(i * stepRPM);
    delay(sweepSpeed);
  }
  delay(sweepSpeed);

  // ramp down
  for (int i = maxRPM; i > 0; i--) {  // set at >0 to stop the needle 'bouncing' when it returns to zero
    setFrequencySpeed(i * stepSpeed);
    setFrequencyRPM(i * stepRPM);
    delay(sweepSpeed);
  }

  delay(sweepSpeed);  // hold at max RPM (to stop immediate return)

  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(frequencyRPM);
  setFrequencySpeed(frequencySpeed);

  delay(sweepSpeed);

#if serialDebug
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

void checkError() {
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