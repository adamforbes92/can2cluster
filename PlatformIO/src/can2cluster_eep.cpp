#include "can2cluster_eep.h"

void readEEP() {
  DEBUG_EEP("EEPROM initialising!");

  pref.begin("can2cluster", false);

  // First boot: seed NVS with current defaults.
  if (!pref.isKey("useHall")) {
    DEBUG_EEP("First run...");
    pref.putBool("useHall", useHall);
    pref.putBool("useVR", useVR);
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
    pref.putUShort("maxFreqVR", maxFreqVR);

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

    for (uint8_t i = 1; i <= 6; i++) {
      String ratioKey = "dsgRatio" + String(i);
      pref.putFloat(ratioKey.c_str(), dsgGearRatio[i]);
    }
    pref.putFloat("dsgFinal14", dsgFinalDrive14);
    pref.putFloat("dsgFinal56", dsgFinalDrive56);
    pref.putFloat("dsgTireCirc", dsgTireCirc);

    pref.putUChar("coolOut", coolantOutput);
    pref.putUInt("coolFreq", coolantPwmFreq);
    pref.putUChar("coolWarn", coolantWarnTemp);
    pref.putUChar("coolCnt", coolantCalCount);
    pref.putBytes("coolTemp", coolantCalTemp, sizeof(coolantCalTemp));
    pref.putBytes("coolDuty", coolantCalDuty, sizeof(coolantCalDuty));

  } else {

    useHall = pref.getBool("useHall", true);
    useVR = pref.getBool("useVR", false);
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
    maxFreqVR = pref.getUShort("maxFreqVR", 200);

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

    for (uint8_t i = 1; i <= 6; i++) {
      String ratioKey = "dsgRatio" + String(i);
      float loaded = pref.getFloat(ratioKey.c_str(), dsgGearRatio[i]);
      if (loaded > 0.0f) dsgGearRatio[i] = loaded;
    }
    dsgFinalDrive14 = pref.getFloat("dsgFinal14", dsgFinalDrive14);
    dsgFinalDrive56 = pref.getFloat("dsgFinal56", dsgFinalDrive56);
    dsgTireCirc = pref.getFloat("dsgTireCirc", dsgTireCirc);

    coolantOutput = pref.getUChar("coolOut", 0);
    coolantPwmFreq = pref.getUInt("coolFreq", 10000);
    coolantWarnTemp = pref.getUChar("coolWarn", 120);
    coolantCalCount = pref.getUChar("coolCnt", 0);
    if (coolantCalCount > COOLANT_CAL_MAX) coolantCalCount = 0;
    pref.getBytes("coolTemp", coolantCalTemp, sizeof(coolantCalTemp));
    pref.getBytes("coolDuty", coolantCalDuty, sizeof(coolantCalDuty));
  }
#if serialDebugEEP
  DEBUG_EEP("EEPROM initialised with...");
  DEBUG_EEP("useHall: %d", useHall);
  DEBUG_EEP("useECU: %d", useECU);
  DEBUG_EEP("useDSG: %d", useDSG);
  DEBUG_EEP("useGPS: %d", useGPS);
  DEBUG_EEP("useABS: %d", useABS);
  DEBUG_EEP("useTP20: %d", useTP20);
  DEBUG_EEP("useHallRPM: %d", useHallRPM);
  DEBUG_EEP("coilType: %d", coilType);
  DEBUG_EEP("useEMLShiftLight: %d", useEMLShiftLight);
  DEBUG_EEP("useEPCShiftLight: %d", useEPCShiftLight);
  DEBUG_EEP("hasNeedleSweep: %d", hasNeedleSweep);
  DEBUG_EEP("clusterRPMLimit: %d", clusterRPMLimit);
  DEBUG_EEP("shiftLimit: %d", shiftLimit);
  DEBUG_EEP("shiftFlashes: %d", shiftFlashes);
  DEBUG_EEP("sweepSpeed: %d", sweepSpeed);
  DEBUG_EEP("maxSpeed: %d", maxSpeed);
  DEBUG_EEP("maxRPM: %d", maxRPM);
  DEBUG_EEP("maxFreqHall: %d", maxFreqHall);
  DEBUG_EEP("stepRPM: %d", stepRPM);
  DEBUG_EEP("stepSpeed: %d", stepSpeed);
  DEBUG_EEP("testReverse: %d", testReverse);
  DEBUG_EEP("testEML: %d", testEML);
  DEBUG_EEP("testEPC: %d", testEPC);
  DEBUG_EEP("gpsUpdateRateHz: %d", gpsUpdateRateHz);
  DEBUG_EEP("broadcastSpeedEnabled: %d", broadcastSpeedEnabled);
  DEBUG_EEP("broadcastSpeedID: 0x%03X", broadcastSpeedID);
  DEBUG_EEP("broadcastSpeedDLC: %d", broadcastSpeedDLC);
  DEBUG_EEP("broadcastSpeedLowByte: %d", broadcastSpeedLowByte);
  DEBUG_EEP("broadcastSpeedHighByte: %d", broadcastSpeedHighByte);
  DEBUG_EEP("broadcastSpeedLittleEndian: %d", broadcastSpeedLittleEndian);
  DEBUG_EEP("broadcastSpeedScale: %.3f", broadcastSpeedScale);
  DEBUG_EEP("broadcastSpeedOffset: %d", broadcastSpeedOffset);
  DEBUG_EEP("dsgParkMode: %s", dsgParkMode.c_str());
  DEBUG_EEP("autoDiagQuery: %d", autoDiagQuery);
#endif
}

void writeEEP(void *args) {
  while (1) {
#if detailedDebugStack
    stackWriteEEP = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

    DEBUG_EEP("Writing EEPROM...");

    pref.putBool("useHall", useHall);
    pref.putBool("useVR", useVR);
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
    pref.putUShort("maxFreqVR", maxFreqVR);

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

    for (uint8_t i = 1; i <= 6; i++) {
      String ratioKey = "dsgRatio" + String(i);
      pref.putFloat(ratioKey.c_str(), dsgGearRatio[i]);
    }
    pref.putFloat("dsgFinal14", dsgFinalDrive14);
    pref.putFloat("dsgFinal56", dsgFinalDrive56);
    pref.putFloat("dsgTireCirc", dsgTireCirc);

    pref.putUChar("coolOut", coolantOutput);
    pref.putUInt("coolFreq", coolantPwmFreq);
    pref.putUChar("coolWarn", coolantWarnTemp);
    pref.putUChar("coolCnt", coolantCalCount);
    pref.putBytes("coolTemp", coolantCalTemp, sizeof(coolantCalTemp));
    pref.putBytes("coolDuty", coolantCalDuty, sizeof(coolantCalDuty));

#if serialDebugEEP
    DEBUG_EEP("Written EEPROM with data:");
    DEBUG_EEP("useHall: %d", useHall);
    DEBUG_EEP("useECU: %d", useECU);
    DEBUG_EEP("useDSG: %d", useDSG);
    DEBUG_EEP("useGPS: %d", useGPS);
    DEBUG_EEP("useABS: %d", useABS);
    DEBUG_EEP("useTP20: %d", useTP20);
    DEBUG_EEP("useHallRPM: %d", useHallRPM);
    DEBUG_EEP("coilType: %d", coilType);
    DEBUG_EEP("useEMLShiftLight: %d", useEMLShiftLight);
    DEBUG_EEP("useEPCShiftLight: %d", useEPCShiftLight);
    DEBUG_EEP("hasNeedleSweep: %d", hasNeedleSweep);
    DEBUG_EEP("clusterRPMLimit: %d", clusterRPMLimit);
    DEBUG_EEP("shiftLimit: %d", shiftLimit);
    DEBUG_EEP("shiftFlashes: %d", shiftFlashes);
    DEBUG_EEP("sweepSpeed: %d", sweepSpeed);
    DEBUG_EEP("maxSpeed: %d", maxSpeed);
    DEBUG_EEP("maxRPM: %d", maxRPM);
    DEBUG_EEP("maxFreqHall: %d", maxFreqHall);
    DEBUG_EEP("stepRPM: %d", stepRPM);
    DEBUG_EEP("stepSpeed: %d", stepSpeed);
    DEBUG_EEP("testReverse: %d", testReverse);
    DEBUG_EEP("testEML: %d", testEML);
    DEBUG_EEP("testEPC: %d", testEPC);
    DEBUG_EEP("gpsUpdateRateHz: %d", gpsUpdateRateHz);
    DEBUG_EEP("broadcastSpeedEnabled: %d", broadcastSpeedEnabled);
    DEBUG_EEP("broadcastSpeedID: 0x%03X", broadcastSpeedID);
    DEBUG_EEP("broadcastSpeedDLC: %d", broadcastSpeedDLC);
    DEBUG_EEP("broadcastSpeedLowByte: %d", broadcastSpeedLowByte);
    DEBUG_EEP("broadcastSpeedHighByte: %d", broadcastSpeedHighByte);
    DEBUG_EEP("broadcastSpeedLittleEndian: %d", broadcastSpeedLittleEndian);
    DEBUG_EEP("broadcastSpeedScale: %.3f", broadcastSpeedScale);
    DEBUG_EEP("broadcastSpeedOffset: %d", broadcastSpeedOffset);
    DEBUG_EEP("dsgParkMode: %s", dsgParkMode.c_str());
    DEBUG_EEP("autoDiagQuery: %d", autoDiagQuery);
#endif
    vTaskDelay(pdMS_TO_TICKS(eepRefresh));
  }
}
