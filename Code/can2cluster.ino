/* 
CAN-BUS converter to Digital Output.  Used for MK2/MK3 'analog' clusters in ME7.x and aftermarket conversions and will provide an EML/EPC light.
All outputs are configurable 12v Square Wave with definable max limits based on x RPM / Speed etc.  All outputs are 200mA max(!).  Reverse is MOSFET and 5A max(!).
Supports GPS for speed

Forbes-Automotive, 2025
*/

// for CAN
#include "can2cluster_defs.h"
ESP32_CAN<RX_SIZE_256, TX_SIZE_16> chassisCAN;

// for GPS
SoftwareSerial ss(pinRX_GPS, pinTX_GPS);
TinyGPSPlus gps;

// for tickers
TickTwo tickError(checkError, 500);              // timer for error checking
TickTwo tickBroadcastSpeed(broadcastSpeed, 20);  // timer for error checking
TickTwo tickEEP(writeEEP, eepRefresh);
TickTwo tickWiFi(disconnectWifi, wifiDisable);  // timer for disconnecting wifi after 30s if no connections - saves power

Preferences pref;

// for inputs / paddles
buttonClass btnPadUp(pinPaddleUp, 0, true);
buttonClass btnPadDown(pinPaddleDown, 0, true);

// define two hardware timers for RPM & Speed outputs
hw_timer_t* timer0 = NULL;
hw_timer_t* timer1 = NULL;

bool rpmTrigger = true;
bool speedTrigger = true;
long frequencyRPM = 20;    // 20 to 20000
long frequencySpeed = 20;  // 20 to 20000

//if (1) {  // This contains all the timers/Hz/Freq. stuff.  Literally in a //(1) to let Arduino IDE code-wrap all this...
// timer for RPM
void IRAM_ATTR onTimer0() {
  rpmTrigger = !rpmTrigger;  // flip-flop to create 50% DC pulse
  if (coilType) {
    digitalWrite(pinCoil, rpmTrigger);  // if 'use coil', trigger the coil
  } else {
    digitalWrite(pinCoil, LOW);        // if NOT 'use coil', so turn off the coil (save power)
    digitalWrite(pinRPM, rpmTrigger);  // if NOT 'use coil', trigger the square wave RPM output
  }
}

// timer for Speed
void IRAM_ATTR onTimer1() {
  speedTrigger = !speedTrigger;          // flip-flop to create 50% DC pulse
  digitalWrite(pinSpeed, speedTrigger);  // trigger speed output
}

// setup timers
void setupTimer() {
  timer0 = timerBegin(0, 40, true);  // used to be div 80
  timerAttachInterrupt(timer0, &onTimer0, true);

  timer1 = timerBegin(1, 40, true);  //used to be div 80 - 40 results in perfect hz transmission
  timerAttachInterrupt(timer1, &onTimer1, true);
}
//}

void setup() {
  basicInit();   // basic init for setting up Serial / IO / CAN / GPS
  setupTimer();  // setup the timers (with a base frequency)

  tickError.start();           // begin the error ticker (for blinking onboard LED)
  tickBroadcastSpeed.start();  // begin ticker for broadcasting speed to CAN
  tickEEP.start();             // begin ticker for the EEPROM
  tickWiFi.start();            // begin ticker for the WiFi (to turn off after 60s)

  if (hasNeedleSweep) {
    needleSweep();  // carry out needle sweep if defined
  }

  connectWifi();         // enable / start WiFi
  WiFi.setSleep(false);  // for the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
  setupUI();             // setup wifi user interface
  //WiFi.setTxPower(WIFI_POWER_8_5dBm);  // set a lower power mode (some C3 aerials aren't great and leaving it high causes failures)
}

void loop() {
  // get the easy stuff out the way first
  tickError.update();           // refresh the Error ticker
  tickBroadcastSpeed.update();  //refresh the Broadcast Speed (via. CAN) ticker
  tickEEP.update();             // refresh the EEP ticker
  tickWiFi.update();            // refresh the WiFi ticker

  btnPadUp.tick();    // refresh the paddle up ticker
  btnPadDown.tick();  // refresh the paddle down ticker

  parseGPS(); // in _gps.ino

  updateLabels();  // in _wifi.ino.  Update the WiFi labels to show current data

  // set EML, EPC & Reverse outputs
  digitalWrite(pinEML, vehicleEML);          // Check for EML light and trigger.  Will be caught by CAN messages (from Motor module)
  digitalWrite(pinEPC, vehicleEPC);          // Check for EPC light and trigger.  Will be caught by CAN messages (from Motor module)
  digitalWrite(pinReverse, vehicleReverse);  // Check for Reverse signal (from DSG) and turn MOSFET on.  Will be caught by CAN messages (from DSG module)

  if (selfTest) {
    diagTest();
  }

  // if last CAN message was >500ms ago, it's in an error state, set flag.  The +10ms is to give a buffer / stop false triggers
  if ((millis() + 10 - lastCAN) > 500) {
    hasError = true;
  } else {
    hasError = false;
  }

  if (tempNeedleSweep) {  // only here if tested in WiFi
#if serialDebug
    DEBUG_PRINTLN("Testing needle sweep");
#endif
    needleSweep();
    //ElegantOTA.begin(ESPUI.server);  // Start ElegantOTA
    tempNeedleSweep = false;  // reset the flag
  }

  // send CAN data for paddle up/down etc
  if (boolPadUp) {
#if serialDebug
    DEBUG_PRINTLN("Paddle up");
#endif
    sendPaddleUpFrame();
    boolPadUp = false;
  }
  if (boolPadDown) {
#if serialDebug
    DEBUG_PRINTLN("Paddle down");
#endif
    sendPaddleDownFrame();
    boolPadDown = false;
  }

  if (testSpeedo) {
#if serialDebug
    DEBUG_PRINTLN("Test speedo");
#endif
    vehicleSpeed = tempSpeed;
  } else {
    if (useHall) {
      if (calcSpeed > 0) {
        vehicleSpeed = (byte)(calcSpeed >= 255 ? 0 : calcSpeed);
      }
    }
    if (useDSG) {
      if ((millis() - lastMillis) > gearPause) {  // check to see if x ms (linPause) has elapsed - slow down the frames!
        lastMillis = millis();
        parseDSG();
      }
      vehicleSpeed = int(dsgSpeed);
    }
    if (useGPS) {
      vehicleSpeed = int(gpsSpeed);
    }
    if (useABS) {
      vehicleSpeed = int(absSpeed);
    }
  }

  if (testRPM) {  // set vehicleRPM is testing or not
#if serialDebug
    DEBUG_PRINTLN("Testing RPM");
#endif
    vehicleRPM = tempRPM;
  } else {
    vehicleRPM = vehicleRPMCAN;
  }

  // check to see what the current RPM is, if it's over the limit, trigger the EPC or EML light as a warning!
  if (useEPCShiftLight || useEMLShiftLight) {
    if (vehicleRPM > shiftLimit) {
      blinkLED(shiftLightRate, shiftFlashes, useEPCShiftLight, useEMLShiftLight, 0, 0);  // args: flash rate, number of flashes, use EPC or use EML as light, RPM/Speed are set to 0, don't use them (kept in for self-test)
    }
  }

  // calculate final frequency:
  frequencySpeed = map(vehicleSpeed, 0, maxSpeed, 0, maxSpeed);
  frequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRPM);

#if serialDebug
  DEBUG_PRINTLN(vehicleRPM);
  DEBUG_PRINTLN(vehicleSpeed);
#endif

  // change the frequency of both RPM & Speed as per CAN information
  if ((millis() - lastMillis2) > rpmPause) {  // check to see if x ms (linPause) has elapsed - slow down the frames!
    lastMillis2 = millis();
    setFrequencyRPM(frequencyRPM);      // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
    setFrequencySpeed(frequencySpeed);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'  }
  }
}
