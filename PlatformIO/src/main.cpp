/* 
CAN-BUS converter to Digital Output.  Used for MK2/MK3 'analog' clusters in ME7.x and aftermarket conversions and will provide an EML/EPC light.
All outputs are configurable 12v Square Wave with definable max limits based on x RPM / Speed etc.  All outputs are 200mA max(!).  Reverse is MOSFET and 5A max(!).

Main features:
> 12v Positive output for reverse light (5A max!)
> RPM/Speed/EML/EPC outputs (200mA max!)
> GPS input for speedo and odometer (1Hz or 5Hz update)
> DSG Paddles (ground to activate), or an MK4 cruise stalk sent to a PQ ECU (GRA 0x38A)
> Optional PQ gateway frames (0x570 / 0x390 / 0x7D0) for cars without the right gateway
> Needle sweep & shift light
> WiFi config
> SavvyCAN / GVRET implementation for CAN analysis (TCP:23 or Serial 1Mbaud)
> DSG / TP2.0 & UDS Support
> Two boards from one codebase (pick the env in platformio.ini):
    ESP32 DevKit board - coolant gauge borrows the EML or EPC output
    ESP32-S3 board     - dedicated coolant gauge output (MCP4725 current sink)

Forbes-Automotive, 2025-2026
*/

#include "can2cluster_defs.h"
#include "can2cluster_gps.h"
#include "can2cluster_io.h"
#include "can2cluster_savvycan.h"
#include "power_manager.h"
#include "wifi_manager.h"

// for GPS
HardwareSerial ss(2); // UART2 for GPS (NEO-6M)
TinyGPSPlus gps;

// for EEPROM
Preferences pref;

// for inputs / paddles - read every 10 ms by the broadcastPaddles task (can2cluster_can.cpp)
OneButton btnPadUp(pinPaddleUp, true, true);     // active-low with internal pull-up
OneButton btnPadDown(pinPaddleDown, true, true); // active-low with internal pull-up

void setup() {
  basicInit();   // Serial, saved settings and the IO pins - everything the needle sweep needs

  if (hasNeedleSweep) {
    startNeedleSweep(); // sweep the needles straight away - it runs in its own task while the rest starts up
  }

  finishInit();  // the slower start-up jobs: I2C, GPS search (up to 3 s with no GPS fitted) and CAN
  setupTasks();  // setup tasks for each of the main functions - CAN Chassis/Haldex handling, Serial prints, Standalone, etc - in '_tasks.cpp'

  connectWifi();         // enable / start WiFi
  setupUI();             // setup wifi user interface
  setupAnalyzer();       // SavvyCAN/GVRET analyzer (TCP:23 or Serial 1Mbaud)

  // Universal reduced-power module: turns WiFi off 1 min after the last client
  // disconnects, scales the CPU 240->80 MHz, releases Bluetooth and kills the
  // onboard LED to cut current draw (and therefore linear-regulator heat).
  power_config_t pcfg = powerDefaultConfig();
  pcfg.verbose = serialDebugWifi; // honour the project's WiFi debug gating
#if !hasOnboardLED
  pcfg.turnOffOnboardLed = false; // S3 board has no onboard LED (GPIO2 is a spare pin there)
#endif
  powerInit(&pcfg);
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
    DEBUG_GPS("[Auto] Rate apply %s: %s", ok ? "OK" : "FAILED", resp.c_str());
  }

  // (the paddles are read by the broadcastPaddles task, not here)

  // "Test Needle Sweep" on the web page
  if (tempNeedleSweep) {
    tempNeedleSweep = false;
    startNeedleSweep(); // runs in its own task, like the power-up sweep
  }

  wifiManagerTick(); // Home WiFi (bridge mode): connection tracking + retry back-off

  delay(10);
}
