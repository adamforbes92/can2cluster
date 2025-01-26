/* 
Basic CAN-BUS converter to Digital Output.  Used for MK2/MK3 'analog' clusters in ME7.x and aftermarket conversions and will provide an EML/EPC light.
All outputs are configurable between 5/12v Square Wave with definable max limits based on x RPM
Optional 'traditional' coil output
Optional EML/EPC output.  EPC can be used as 'shift light', RPM configarble
Original RPM input is ~500Hz, speed is ~300Hz for VW Clusters.  Adjustable in code
Optional GPS module for calculating speed if ECU is blind.  Not as accurate but a valid solution...
Built-in LED used for error displaying.  For example - no satellites will illuminate LED
Added DSG support - gets current gear & rpm and calculates theory speed.  Ratios in '_dsg.ino'

Forbes-Automotive, 2025
*/

// for CAN
#include "canbus2cluster_defs.h"
#include <ESP32_CAN.h>
ESP32_CAN<RX_SIZE_256, TX_SIZE_16> chassisCAN;

// for GPS
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
SoftwareSerial ss(pinRX_GPS, pinTX_GPS);
TinyGPSPlus gps;

// for inputs / paddles
#include <ButtonLib.h>  //include the declaration for this class
buttonClass btnPadUp(pinPaddleUp, true);
buttonClass btnPadDown(pinPaddleDown, true);
buttonClass btnSpare1(pinSpare1, true);
buttonClass btnSpare2(pinSpare2, true);

// define two hardware timers for RPM & Speed outputs
hw_timer_t* timer0 = NULL;
hw_timer_t* timer1 = NULL;

bool rpmTrigger = true;
bool speedTrigger = true;
int frequencyRPM = 20;    // 20 to 20000
int frequencySpeed = 20;  // 20 to 20000

//if (1) {  // This contains all the timers/Hz/Freq. stuff.  Literally in a //(1) to let Arduino IDE code-wrap all this...
  // timer for RPM
  void IRAM_ATTR onTimer0() {
    rpmTrigger = !rpmTrigger;
    digitalWrite(pinRPM, rpmTrigger);
    if (hasCoilOutput) {
      digitalWrite(pinCoil, rpmTrigger);
    }
  }

  // timer for Speed
  void IRAM_ATTR onTimer1() {
    speedTrigger = !speedTrigger;
    digitalWrite(pinSpeed, speedTrigger);
  }

  // setup timers
  void setupTimer() {
    timer0 = timerBegin(0, 80, true);  //div 80
    timerAttachInterrupt(timer0, &onTimer0, true);

    timer1 = timerBegin(1, 80, true);  //div 80
    timerAttachInterrupt(timer1, &onTimer1, true);
  }

  // adjust output frequency
  void setFrequencyRPM(long frequencyHz) {
    timerAlarmDisable(timer0);
    timerAlarmWrite(timer0, 1000000l / frequencyHz, true);
    timerAlarmEnable(timer0);
  }

  // adjust output frequency
  void setFrequencySpeed(long frequencyHz) {
    timerAlarmDisable(timer1);
    timerAlarmWrite(timer1, 1000000l / frequencyHz, true);
    timerAlarmEnable(timer1);
  }
//}

void setup() {
  basicInit();   // basic init for setting up IO / CAN / GPS
  setupTimer();  // setup the timers (with a base frequency)

  if (hasNeedleSweep) {
    needleSweep();  // carry out needle sweep if defined
  }
}

void loop() {
  // get the easy stuff out the way first
  btnPadUp.tick();    // using the 'button' library to detect held or single clicks
  btnPadDown.tick();  // using the 'button' library to detect held or single clicks
  btnSpare1.tick();   // using the 'button' library to detect held or single clicks
  btnSpare2.tick();   // using the 'button' library to detect held or single clicks

  // send CAN data for paddle up/down etc

  if (hasGPSSpeed) {  // GPS if req.
    parseGPS();
  }

  digitalWrite(pinEML, vehicleEML);  // Check for EML/EPC light and trigger.  Will be caught by CAN messages
  digitalWrite(pinEPC, vehicleEPC);  // Check for EML/EPC light and trigger.  Will be caught by CAN messages

  // check to see what the current RPM is, if it's over the limit, trigger the EPC or EML light as a warning!
  if (vehicleRPM > shiftLimit) {
    blinkLED(shiftLightRate, 3, useEPCShiftLight, useEMLShiftLight, 0, 0);  // args: flash rate, number of flashes, use EPC or use EML as light, RPM/Speed are set to 0, don't use them (kept in for self-test)
  }

  if (hasError) {
    digitalWrite(2, HIGH); // light internal LED
  } else {
    digitalWrite(2, LOW);
  }

  if (selfTest) {
    diagTest();
  } else {
    // calculate final frequency:
    finalFrequencySpeed = map(vehicleSpeed, 0, clusterSpeedLimit, 0, maxSpeed);
    finalFrequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRRM);

    // change the frequency of both RPM & Speed as per CAN information
    setFrequencySpeed(finalFrequencySpeed + 1);  // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
    setFrequencyRPM(finalFrequencyRPM + 1);      // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
  }
}