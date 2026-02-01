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

Preferences pref;

// for inputs / paddles
InterruptButton btnPadUp(pinPaddleUp, LOW, GPIO_MODE_INPUT, 1000, 500, 750, 80000);      // pin, GPIO_MODE_INPUT, state when pressed, long press, autorepeat, double-click, debounce
InterruptButton btnPadDown(pinPaddleDown, LOW, GPIO_MODE_INPUT, 1000, 500, 750, 80000);  // pin, GPIO_MODE_INPUT, state when pressed, long press, autorepeat, double-click, debounce

ESPAsyncHTTPUpdateServer updateServer;  // for the OTA updates

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

  timer1 = timerBegin(1, 40, true);  // used to be div 80 - 40 results in perfect hz transmission
  timerAttachInterrupt(timer1, &onTimer1, true);
}
//}

void setup() {
  basicInit();   // basic init for setting up Serial / IO / CAN / GPS
  setupTimer();  // setup the timers (with a base frequency)
  setupTasks();  // setup the tasks
  setupWiFi();                       // enable / start WiFi
  setupUI();                           // setup wifi user interface
  setupOTA();                          // setup Over-the-Air updates

  if (hasNeedleSweep) {
    vTaskSuspend(handle_processOutputs);
    needleSweep();  // carry out needle sweep if defined
    vTaskResume(handle_processOutputs);
  }
}

void loop() {
  // for small tasks not running all the time
  if (tempNeedleSweep) {  // only here if tested in WiFi
    vTaskSuspend(handle_processOutputs);
    needleSweep();  // carry out needle sweep if defined
    vTaskResume(handle_processOutputs);
    tempNeedleSweep = false;  // reset the flag
  }

  if (tempShiftLight) {  // only here if tested in WiFi
    vTaskSuspend(handle_parseShiftLights);
    if (useEPCShiftLight || useEMLShiftLight) {
      blinkLED(shiftLightRate, shiftFlashes, useEPCShiftLight, useEMLShiftLight, 0, 0);  // args: flash rate, number of flashes, use EPC or use EML as light, RPM/Speed are set to 0, don't use them (kept in for self-test)
    }
    vTaskResume(handle_parseShiftLights);
    tempShiftLight = false;  // reset the flag
  }

  if ((millis() + 10 - lastCAN) > 500) {
    hasError = true;
  } else {
    hasError = false;
  }
}
