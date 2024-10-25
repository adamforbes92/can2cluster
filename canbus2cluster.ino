/* 
Basic CAN-BUS converter to Digital Output.  Used for MK2/MK3 'analog' clusters in ME7.x conversions and will provide an EML/EPC light.
All outputs are configurable between 5/12v Square Wave with definable max limits based on x RPM
Optional 'traditional' coil output
Optional EML/EPC output.  EPC can be used as 'shift light', RPM configarble
Original RPM input is ~500Hz, speed is ~300Hz for VW Clusters.  Adjustable in code

Forbes-Automotive, 2024
*/

#include "canbus2cluster_defs.h"
#include <ESP32_CAN.h>  // for CAN
ESP32_CAN<RX_SIZE_256, TX_SIZE_16> chassisCAN;

// define two hardware timers for RPM & Speed outputs
hw_timer_t* timer0 = NULL;
hw_timer_t* timer1 = NULL;

bool rpmTrigger = true;
bool speedTrigger = true;
int frequencyRPM = 20;    // 20 to 20000
int frequencySpeed = 20;  // 20 to 20000

// temporary status' for EML/ABS/RPM/Speed
bool vehicleEML = false;
bool vehicleEPC = false;
int vehicleRPM = 0;
int vehicleSpeed = 0;
int calcSpeed = 0;

// timer for RPM
void IRAM_ATTR onTimer0() {
  rpmTrigger = !rpmTrigger;
  digitalWrite(pinRPM, rpmTrigger);
#if hasCoilOutput
  digitalWrite(pinCoil, rpmTrigger);
#endif
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

void setup() {
  basicInit();   // basic init for setting up IO / CAN
  setupTimer();  // setup the timers (with a base frequency)

#if (hasNeedleSweep)
  needleSweep();  // carry out needle sweep if defined
#endif
}

void loop() {
  // get the easy stuff out the way first, check for EML/EPC light and trigger if required
  digitalWrite(pinEML, vehicleEML);
  digitalWrite(pinEPC, vehicleEPC);

  // check to see what the current RPM is, if it's over the limit, trigger the EPC or EML light as a warning!
  if (vehicleRPM > shiftLimit) {
    blinkLED(shiftLightRate, 3, useEPCShiftLight, useEMLShiftLight);  // args: flash rate, number of flashes, use EPC / EML as light
  }

  if (selfTest) {
    int finalFrequencySpeed = map(finalFrequencySpeed, 0, vehicleSpeed, 0, maxSpeed);
    int finalFrequencyRPM = map(finalFrequencyRPM, 0, vehicleRPM, 0, maxRRM);

    // change the frequency of both RPM & Speed as per CAN information
    setFrequencySpeed(finalFrequencySpeed);
    setFrequencyRPM(finalFrequencyRPM);
  } else {
    // calculate final frequency:
    int finalFrequencySpeed = map(finalFrequencySpeed, 0, vehicleSpeed, 0, maxSpeed);
    int finalFrequencyRPM = map(finalFrequencyRPM, 0, vehicleRPM, 0, maxRRM);

    // change the frequency of both RPM & Speed as per CAN information
    setFrequencySpeed(finalFrequencySpeed);
    setFrequencyRPM(finalFrequencyRPM);
  }
}