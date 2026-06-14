#include "can2cluster_eep.h"

void readEEP() {
#if serialDebugEEP
  DEBUG("EEPROM initialising!");
#endif

  pref.begin("can2cluster", false);

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
    pref.putBool("useTP20", useTP20);
    pref.putBool("useUDS", useUDS);
    pref.putBool("useHallRPM", useHallRPM);
    pref.putBool("coilType", coilType);
    pref.putBool("useMPH", useMPH);
    pref.putBool("diagTest", diagTest);
    pref.putBool("analyzerMode", analyzerMode);
    pref.putBool("analyzerSerial", analyzerSerial);
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
    pref.putUChar("gpsUpdateRateHz", gpsUpdateRateHz);
    pref.putBool("bsEn", broadcastSpeedEnabled);
    pref.putUInt("bsId", broadcastSpeedID);
    pref.putUChar("bsDlc", broadcastSpeedDLC);
    pref.putUChar("bsLow", broadcastSpeedLowByte);
    pref.putUChar("bsHigh", broadcastSpeedHighByte);
    pref.putBool("bsLe", broadcastSpeedLittleEndian);
    pref.putFloat("bsScale", broadcastSpeedScale);
    pref.putShort("bsOffset", broadcastSpeedOffset);
    for (uint8_t i = 0; i < 8; i++) {
      String dataKey = "bsD" + String(i);
      pref.putUChar(dataKey.c_str(), broadcastSpeedData[i]);
    }
    pref.putString("dsgParkMode", dsgParkMode);
    pref.putBool("autoDiagQuery", autoDiagQuery);
    pref.putBool("useAftermarket", useAftermarket);
    pref.putUInt("amSpeedID", aftermarketSpeedID);
    pref.putUChar("amSpeedLow", aftermarketSpeedLowByte);
    pref.putUChar("amSpeedHigh", aftermarketSpeedHighByte);
    pref.putBool("amSpeedLE", aftermarketSpeedLittleEndian);
    pref.putFloat("amSpeedScale", aftermarketSpeedScale);
    pref.putShort("amSpeedOffset", aftermarketSpeedOffset);

  } else {

    useHall = pref.getBool("useHall", true);
    useECU = pref.getBool("useECU", false);
    useDSG = pref.getBool("useDSG", false);
    useGPS = pref.getBool("useGPS", false);
    useABS = pref.getBool("useABS", false);
    useTP20 = pref.getBool("useTP20", false);
    useUDS  = pref.getBool("useUDS", false);
    useHallRPM = pref.getBool("useHallRPM", false);
    coilType = pref.getBool("coilType", true);
    useMPH = pref.getBool("useMPH", false);
    diagTest = pref.getBool("diagTest", false);
    analyzerMode   = pref.getBool("analyzerMode", false);
    analyzerSerial = pref.getBool("analyzerSerial", false);
    useEMLShiftLight = pref.getBool("useEMLLight", false);
    useEPCShiftLight = pref.getBool("useEPCLight", false);

    hasNeedleSweep = pref.getBool("hasNeedleSweep", false);
    
    // Enable UDS/TP diagnostics only if TP2.0 is selected
    autoDiagQuery = useTP20 || useUDS;

    clusterRPMLimit = pref.getUShort("clusterRPMLimit", 7000);
    shiftLimit = pref.getUShort("shiftLimit", 6000);
    shiftFlashes = pref.getUChar("shiftFlashes", 3);
    sweepSpeed = pref.getUChar("sweepSpeed", 18);
    maxSpeed = pref.getUShort("maxSpeed", 200);
    maxRPM = pref.getUShort("maxRPM", 230);
    maxFreqHall = pref.getUShort("maxFreqHall", 200);

    stepRPM = pref.getUShort("stepRPM", 12);
    stepSpeed = pref.getUShort("stepSpeed", 10);

    // Migrate legacy step-based values: old approach used large increments (e.g. 100 Hz/step).
    // The new time-based formula treats these as duration multipliers (10 = normal, like SPP).
    if (stepSpeed > 50)  stepSpeed = 10;
    if (stepRPM   > 50)  stepRPM   = 12;
    if (sweepSpeed > 100 || sweepSpeed < 1) sweepSpeed = 18;
    
    testReverse = pref.getBool("testReverse", false);
    testEML = pref.getBool("testEML", false);
    testEPC = pref.getBool("testEPC", false);
    gpsUpdateRateHz = pref.getUChar("gpsUpdateRateHz", 1);
    broadcastSpeedEnabled = pref.getBool("bsEn", false);
    broadcastSpeedID = pref.getUInt("bsId", MOTOR2_ID) & 0x7FF;
    broadcastSpeedDLC = pref.getUChar("bsDlc", 8);
    broadcastSpeedLowByte = pref.getUChar("bsLow", 3);
    broadcastSpeedHighByte = pref.getUChar("bsHigh", 2);
    broadcastSpeedLittleEndian = pref.getBool("bsLe", false);
    broadcastSpeedScale = pref.getFloat("bsScale", 1.0f);
    broadcastSpeedOffset = pref.getShort("bsOffset", 0);
    for (uint8_t i = 0; i < 8; i++) {
      String dataKey = "bsD" + String(i);
      broadcastSpeedData[i] = pref.getUChar(dataKey.c_str(), 0);
    }
    dsgParkMode = pref.getString("dsgParkMode", "None");
    autoDiagQuery = pref.getBool("autoDiagQuery", useTP20 || useUDS);
    useAftermarket = pref.getBool("useAftermarket", false);
    aftermarketSpeedID = pref.getUInt("amSpeedID", 0x200) & 0x7FF;
    aftermarketSpeedLowByte = pref.getUChar("amSpeedLow", 0);
    aftermarketSpeedHighByte = pref.getUChar("amSpeedHigh", 1);
    aftermarketSpeedLittleEndian = pref.getBool("amSpeedLE", true);
    aftermarketSpeedScale = pref.getFloat("amSpeedScale", 1.0f);
    aftermarketSpeedOffset = pref.getShort("amSpeedOffset", 0);
  }
#if serialDebugEEP
  DEBUG("EEPROM initialised with...");
  DEBUG("useHall: %d", useHall);
  DEBUG("useECU: %d", useECU);
  DEBUG("useDSG: %d", useDSG);
  DEBUG("useGPS: %d", useGPS);
  DEBUG("useABS: %d", useABS);
  DEBUG("useTP20: %d", useTP20);
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
  DEBUG("gpsUpdateRateHz: %d", gpsUpdateRateHz);
  DEBUG("broadcastSpeedEnabled: %d", broadcastSpeedEnabled);
  DEBUG("broadcastSpeedID: 0x%03X", broadcastSpeedID);
  DEBUG("broadcastSpeedDLC: %d", broadcastSpeedDLC);
  DEBUG("broadcastSpeedLowByte: %d", broadcastSpeedLowByte);
  DEBUG("broadcastSpeedHighByte: %d", broadcastSpeedHighByte);
  DEBUG("broadcastSpeedLittleEndian: %d", broadcastSpeedLittleEndian);
  DEBUG("broadcastSpeedScale: %.3f", broadcastSpeedScale);
  DEBUG("broadcastSpeedOffset: %d", broadcastSpeedOffset);
  DEBUG("dsgParkMode: %s", dsgParkMode.c_str());
  DEBUG("autoDiagQuery: %d", autoDiagQuery);
#endif
}

void writeEEP(void *args) {
  while (1) {
#if detailedDebugStack
    stackWriteEEP = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

#if serialDebugEEP
    DEBUG("Writing EEPROM...");
#endif

    pref.putBool("useHall", useHall);
    pref.putBool("useECU", useECU);
    pref.putBool("useDSG", useDSG);
    pref.putBool("useGPS", useGPS);
    pref.putBool("useABS", useABS);
    pref.putBool("useTP20", useTP20);
    pref.putBool("useUDS", useUDS);
    pref.putBool("useHallRPM", useHallRPM);
    pref.putBool("coilType", coilType);
    pref.putBool("useMPH", useMPH);
    pref.putBool("diagTest", diagTest);
    pref.putBool("useEMLLight", useEMLShiftLight);
    pref.putBool("analyzerMode", analyzerMode);
    pref.putBool("analyzerSerial", analyzerSerial);
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
    pref.putUChar("gpsUpdateRateHz", gpsUpdateRateHz);
    pref.putBool("bsEn", broadcastSpeedEnabled);
    pref.putUInt("bsId", broadcastSpeedID);
    pref.putUChar("bsDlc", broadcastSpeedDLC);
    pref.putUChar("bsLow", broadcastSpeedLowByte);
    pref.putUChar("bsHigh", broadcastSpeedHighByte);
    pref.putBool("bsLe", broadcastSpeedLittleEndian);
    pref.putFloat("bsScale", broadcastSpeedScale);
    pref.putShort("bsOffset", broadcastSpeedOffset);
    for (uint8_t i = 0; i < 8; i++) {
      String dataKey = "bsD" + String(i);
      pref.putUChar(dataKey.c_str(), broadcastSpeedData[i]);
    }
    pref.putString("dsgParkMode", dsgParkMode);
    pref.putBool("autoDiagQuery", autoDiagQuery);
    pref.putBool("useAftermarket", useAftermarket);
    pref.putUInt("amSpeedID", aftermarketSpeedID);
    pref.putUChar("amSpeedLow", aftermarketSpeedLowByte);
    pref.putUChar("amSpeedHigh", aftermarketSpeedHighByte);
    pref.putBool("amSpeedLE", aftermarketSpeedLittleEndian);
    pref.putFloat("amSpeedScale", aftermarketSpeedScale);
    pref.putShort("amSpeedOffset", aftermarketSpeedOffset);

#if serialDebugEEP
    DEBUG("Written EEPROM with data:");
    DEBUG("useHall: %d", useHall);
    DEBUG("useECU: %d", useECU);
    DEBUG("useDSG: %d", useDSG);
    DEBUG("useGPS: %d", useGPS);
    DEBUG("useABS: %d", useABS);
    DEBUG("useTP20: %d", useTP20);
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
    DEBUG("gpsUpdateRateHz: %d", gpsUpdateRateHz);
    DEBUG("broadcastSpeedEnabled: %d", broadcastSpeedEnabled);
    DEBUG("broadcastSpeedID: 0x%03X", broadcastSpeedID);
    DEBUG("broadcastSpeedDLC: %d", broadcastSpeedDLC);
    DEBUG("broadcastSpeedLowByte: %d", broadcastSpeedLowByte);
    DEBUG("broadcastSpeedHighByte: %d", broadcastSpeedHighByte);
    DEBUG("broadcastSpeedLittleEndian: %d", broadcastSpeedLittleEndian);
    DEBUG("broadcastSpeedScale: %.3f", broadcastSpeedScale);
    DEBUG("broadcastSpeedOffset: %d", broadcastSpeedOffset);
    DEBUG("dsgParkMode: %s", dsgParkMode.c_str());
    DEBUG("autoDiagQuery: %d", autoDiagQuery);
#endif
    vTaskDelay(pdMS_TO_TICKS(eepRefresh));
  }
}
