#include "can2cluster_eep.h"

namespace {
constexpr const char *kSettingsNamespace = "autoDiagQuery";
bool prefReady = false;
}

void readEEP() {
#if serialDebugEEP
  DEBUG("EEPROM initialising!");
#endif

  // Use one namespace for all keys.
  prefReady = pref.begin(kSettingsNamespace, false);
  if (!prefReady) {
    DEBUG_EEP("Failed to open Preferences namespace: %s", kSettingsNamespace);
    return;
  }

  // First boot: seed NVS with current defaults.
  if (!pref.isKey("useHall")) {
#if serialDebugEEP
    DEBUG("First run...");
#endif
    pref.putBool("useHall", useHall);
    pref.putBool("useECU", useECU);
    pref.putBool("useDSG", useDSG);
    pref.putBool("useGPS", useGPS);
    pref.putBool("useABS", useABS);
    pref.putBool("useTPUDSDSG", useTPUDSDSG);
    pref.putBool("useHallRPM", useHallRPM);
    pref.putBool("coilType", coilType);
    pref.putBool("useEMLLight", useEMLShiftLight);
    pref.putBool("useEPCLight", useEPCShiftLight);

    pref.putBool("hasNeedleSweep", hasNeedleSweep);

    pref.putUShort("clusterRPMLimit", clusterRPMLimit);
    pref.putUShort("shiftLimit", shiftLimit);
    pref.putUChar("shiftFlashes", shiftFlashes);
    pref.putUChar("sweepSpeed", sweepSpeed);
    pref.putUShort("maxSpeed", maxSpeed);
    pref.putUShort("maxRPM", maxRPM);
    pref.putUShort("maxFreqHall", maxFreqHall);

    pref.putUShort("stepRPM", stepRPM);
    pref.putUShort("stepSpeed", stepSpeed);

    pref.putBool("testReverse", testReverse);
    pref.putBool("testEML", testEML);
    pref.putBool("testEPC", testEPC);
    pref.putString("dsgParkMode", dsgParkMode);
    pref.putBool("autoDiagQuery", autoDiagQuery);

  } else {

    useHall = pref.getBool("useHall", true);
    useECU = pref.getBool("useECU", false);
    useDSG = pref.getBool("useDSG", false);
    useGPS = pref.getBool("useGPS", false);
    useABS = pref.getBool("useABS", false);
    useTPUDSDSG = pref.getBool("useTPUDSDSG", false);
    useHallRPM = pref.getBool("useHallRPM", false);
    coilType = pref.getBool("coilType", true);
    useEMLShiftLight = pref.getBool("useEMLLight", false);
    useEPCShiftLight = pref.getBool("useEPCLight", false);

    hasNeedleSweep = pref.getBool("hasNeedleSweep", false);
    
    // Enable UDS/TP diagnostics only if TP/UDS DSG is selected
    autoDiagQuery = useTPUDSDSG;

    clusterRPMLimit = pref.getUShort("clusterRPMLimit", 7000);
    shiftLimit = pref.getUShort("shiftLimit", 6000);
    shiftFlashes = pref.getUChar("shiftFlashes", 3);
    sweepSpeed = pref.getUChar("sweepSpeed", 150);
    maxSpeed = pref.getUShort("maxSpeed", 200);
    maxRPM = pref.getUShort("maxRPM", 230);
    maxFreqHall = pref.getUShort("maxFreqHall", 200);

    stepRPM = pref.getUShort("stepRPM", 12);
    stepSpeed = pref.getUShort("stepSpeed", 10);

    // Migrate legacy/bad persisted values that make sweep appear "stuck".
    if (stepRPM < 10) {
      stepRPM = 100;
    }
    if (stepSpeed < 10) {
      stepSpeed = 100;
    }
    if (sweepSpeed < 1) {
      sweepSpeed = 18;
    }
    
    testReverse = pref.getBool("testReverse", false);
    testEML = pref.getBool("testEML", false);
    testEPC = pref.getBool("testEPC", false);
    dsgParkMode = pref.getString("dsgParkMode", "None");
    autoDiagQuery = pref.getBool("autoDiagQuery", useTPUDSDSG);
  }
#if serialDebugEEP
  DEBUG("EEPROM initialised with...");
  DEBUG("useHall: %d", useHall);
  DEBUG("useECU: %d", useECU);
  DEBUG("useDSG: %d", useDSG);
  DEBUG("useGPS: %d", useGPS);
  DEBUG("useABS: %d", useABS);
  DEBUG("useTPUDSDSG: %d", useTPUDSDSG);
  DEBUG("useHallRPM: %d", useHallRPM);
  DEBUG("coilType: %d", coilType);
  DEBUG("useEMLShiftLight: %d", useEMLShiftLight);
  DEBUG("useEPCShiftLight: %d", useEPCShiftLight);
  DEBUG("hasNeedleSweep: %d", hasNeedleSweep);
  DEBUG("clusterRPMLimit: %d", clusterRPMLimit);
  DEBUG("shiftLimit: %d", shiftLimit);
  DEBUG("shiftFlashes: %d", shiftFlashes);
  DEBUG("sweepSpeed: %d", sweepSpeed);
  DEBUG("maxSpeed: %d", maxSpeed);
  DEBUG("maxRPM: %d", maxRPM);
  DEBUG("maxFreqHall: %d", maxFreqHall);
  DEBUG("stepRPM: %d", stepRPM);
  DEBUG("stepSpeed: %d", stepSpeed);
  DEBUG("testReverse: %d", testReverse);
  DEBUG("testEML: %d", testEML);
  DEBUG("testEPC: %d", testEPC);
  DEBUG("dsgParkMode: %s", dsgParkMode.c_str());
  DEBUG("autoDiagQuery: %d", autoDiagQuery);
#endif
}

void writeEEP(void *args) {
  while (1) {
#if detailedDebugStack
    stackWriteEEP = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

    // Only write to flash when something has changed — prevents progressive NVS wear
    if (!eepDirty) {
      vTaskDelay(pdMS_TO_TICKS(eepRefresh));
      continue;
    }

    if (!prefReady) {
      prefReady = pref.begin(kSettingsNamespace, false);
      if (!prefReady) {
        DEBUG_EEP("Write skipped: Preferences namespace unavailable");
        vTaskDelay(pdMS_TO_TICKS(eepRefresh));
        continue;
      }
    }

    eepDirty = false;

#if serialDebugEEP
    DEBUG("Writing EEPROM...");
#endif

    pref.putBool("useHall", useHall);
    pref.putBool("useECU", useECU);
    pref.putBool("useDSG", useDSG);
    pref.putBool("useGPS", useGPS);
    pref.putBool("useABS", useABS);
    pref.putBool("useTPUDSDSG", useTPUDSDSG);
    pref.putBool("useHallRPM", useHallRPM);
    pref.putBool("coilType", coilType);
    pref.putBool("useEMLLight", useEMLShiftLight);
    pref.putBool("useEPCLight", useEPCShiftLight);

    pref.putBool("hasNeedleSweep", hasNeedleSweep);

    pref.putUShort("clusterRPMLimit", clusterRPMLimit);
    pref.putUShort("shiftLimit", shiftLimit);
    pref.putUChar("shiftFlashes", shiftFlashes);
    pref.putUChar("sweepSpeed", sweepSpeed);
    pref.putUShort("maxSpeed", maxSpeed);
    pref.putUShort("maxRPM", maxRPM);
    pref.putUShort("maxFreqHall", maxFreqHall);

    pref.putUShort("stepRPM", stepRPM);
    pref.putUShort("stepSpeed", stepSpeed);

    pref.putBool("testReverse", testReverse);
    pref.putBool("testEML", testEML);
    pref.putBool("testEPC", testEPC);
    pref.putString("dsgParkMode", dsgParkMode);
    pref.putBool("autoDiagQuery", autoDiagQuery);

#if serialDebugEEP
    DEBUG("Written EEPROM with data:");
    DEBUG("useHall: %d", useHall);
    DEBUG("useECU: %d", useECU);
    DEBUG("useDSG: %d", useDSG);
    DEBUG("useGPS: %d", useGPS);
    DEBUG("useABS: %d", useABS);
    DEBUG("useTPUDSDSG: %d", useTPUDSDSG);
    DEBUG("useHallRPM: %d", useHallRPM);
    DEBUG("coilType: %d", coilType);
    DEBUG("useEMLShiftLight: %d", useEMLShiftLight);
    DEBUG("useEPCShiftLight: %d", useEPCShiftLight);
    DEBUG("hasNeedleSweep: %d", hasNeedleSweep);
    DEBUG("clusterRPMLimit: %d", clusterRPMLimit);
    DEBUG("shiftLimit: %d", shiftLimit);
    DEBUG("shiftFlashes: %d", shiftFlashes);
    DEBUG("sweepSpeed: %d", sweepSpeed);
    DEBUG("maxSpeed: %d", maxSpeed);
    DEBUG("maxRPM: %d", maxRPM);
    DEBUG("maxFreqHall: %d", maxFreqHall);
    DEBUG("stepRPM: %d", stepRPM);
    DEBUG("stepSpeed: %d", stepSpeed);
    DEBUG("testReverse: %d", testReverse);
    DEBUG("testEML: %d", testEML);
    DEBUG("testEPC: %d", testEPC);
    DEBUG("dsgParkMode: %s", dsgParkMode.c_str());
    DEBUG("autoDiagQuery: %d", autoDiagQuery);
#endif
    vTaskDelay(pdMS_TO_TICKS(eepRefresh));
  }
}