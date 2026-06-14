/* 
CAN-BUS converter to Digital Output.  Used for MK2/MK3 'analog' clusters in ME7.x and aftermarket conversions and will provide an EML/EPC light.
All outputs are configurable 12v Square Wave with definable max limits based on x RPM / Speed etc.  All outputs are 200mA max(!).  Reverse is MOSFET and 5A max(!).

Main features:
> 12v Positive output for reverse light (5A max!)
> RPM/Speed/EML/EPC outputs (200mA max!)
> GPS input for speedo and odometer (1Hz or 5Hz update)
> DSG Paddles (ground to activate)
> Needle sweep & shift light
> WiFi config
> SavvyCAN / GVRET implementation for CAN analysis (TCP:23 or Serial 1Mbaud)
> DSG / TP2.0 & UDS Support

Forbes-Automotive, 2025
*/

#include "can2cluster_defs.h"
#include "can2cluster_gps.h"
#include "can2cluster_io.h"
#include "can2cluster_savvycan.h"

// for GPS
SoftwareSerial ss(pinRX_GPS, pinTX_GPS);
TinyGPSPlus gps;

// for EEPROM
Preferences pref;

// for inputs / paddles
OneButton btnPadUp(pinPaddleUp, true, true);     // active-low with internal pull-up
OneButton btnPadDown(pinPaddleDown, true, true); // active-low with internal pull-up

void setup() {
  basicInit();   // basic init for setting up Serial / IO / CAN / GPS
  setupTasks();  // setup tasks for each of the main functions - CAN Chassis/Haldex handling, Serial prints, Standalone, etc - in '_io.ino'

  if (hasNeedleSweep) {
    tasksSuspendAll();
    needleSweep();
    tasksResumeAll();
  }

  connectWifi();         // enable / start WiFi
  setupUI();             // setup wifi user interface
  setupAnalyzer();       // SavvyCAN/GVRET analyzer (TCP:23 or Serial 1Mbaud)
}

void loop() {
  // Apply GPS rate from EEPROM after satellite lock (flagged by parseGPS).
  // Tasks are suspended for the duration so the delay() calls inside
  // setGPSUpdateRate() are not disrupted by higher-priority core-1 tasks.
  // The API path doesn't need this because AsyncTCP callbacks run on core 0.
  if (gpsAutoRateApplyPending())
  {
    tasksSuspendAll();
    String resp;
    bool ok = setGPSUpdateRate(gpsUpdateRateHz, resp);
    tasksResumeAll();
    DEBUG("[GPS Auto] Rate apply %s: %s", ok ? "OK" : "FAILED", resp.c_str());
  }

  updateButtons();

  if (tempNeedleSweep) {
    tasksSuspendAll();
    needleSweep();
    tasksResumeAll();
    tempNeedleSweep = false;
  }

  delay(10);
}
