void readEEP() {
#if serialDebugEEP
  DEBUG_PRINTLN("EEPROM initialising!");
#endif

  // use ESP32's 'Preferences' to remember settings.  Begin by opening the various types.  Use 'false' for read/write.  True just gives read access
  pref.begin("useHall", false);
  pref.begin("useDSG", false);
  pref.begin("useGPS", false);
  pref.begin("useABS", false);
  pref.begin("coilType", false);
  pref.begin("useEMLLight", false);
  pref.begin("useEPCLight", false);

  pref.begin("hasNeedleSweep", false);

  pref.begin("clusterRPMLimit", false);
  pref.begin("shiftLimit", false);
  pref.begin("shiftFlashes", false);
  pref.begin("sweepSpeed", false);
  pref.begin("maxSpeed", false);
  pref.begin("maxRPM", false);
  pref.begin("maxFreqHall", false);

  pref.begin("stepRPM", false);
  pref.begin("stepSpeed", false);

  // first run comes with EEP valve of 255, so write actual values
  if (pref.getUChar("useHall") == 255) {
#if serialDebugEEP
    DEBUG_PRINTLN("First run...");
    DEBUG_PRINTLN(pref.getUChar("useHall"));
#endif
    pref.putBool("useHall", useHall);
    pref.putBool("useDSG", useDSG);
    pref.putBool("useGPS", useGPS);
    pref.putBool("useABS", useABS);
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

  } else {

    useHall = pref.getBool("useHall", true);
    useDSG = pref.getBool("useDSG", false);
    useGPS = pref.getBool("useGPS", false);
    useABS = pref.getBool("useABS", false);
    coilType = pref.getBool("coilType", true);
    useEMLShiftLight = pref.getBool("useEMLLight", false);
    useEPCShiftLight = pref.getBool("useEPCLight", false);

    hasNeedleSweep = pref.getBool("hasNeedleSweep", false);

    clusterRPMLimit = pref.getUShort("clusterRPMLimit", 7000);
    shiftLimit = pref.getUShort("shiftLimit", 6000);
    shiftFlashes = pref.getUChar("shiftFlashes", 3);
    sweepSpeed = pref.getUChar("sweepSpeed", 18);
    maxSpeed = pref.getUShort("maxSpeed", 200);
    maxRPM = pref.getUShort("maxRPM", 230);
    maxFreqHall = pref.getUShort("maxFreqHall", 200);

    stepRPM = pref.getUShort("stepRPM", 1.2);
    stepSpeed = pref.getUShort("stepSpeed", 1);
  }
#if serialDebugEEP
  DEBUG_PRINTLN("EEPROM initialised with...");
  DEBUG_PRINTLN(useEMLShiftLight);
  DEBUG_PRINTLN(useEPCShiftLight);
#endif
}

void writeEEP() {
#if serialDebugEEP
  DEBUG_PRINTLN("Writing EEPROM...");
#endif

  // update EEP only if changes have been made
  pref.putBool("useHall", useHall);
  pref.putBool("useDSG", useDSG);
  pref.putBool("useGPS", useGPS);
  pref.putBool("useABS", useABS);
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

#if serialDebugEEP
  DEBUG_PRINTLN("Written EEPROM with data:");
  DEBUG_PRINTLN(useEMLShiftLight);
  DEBUG_PRINTLN(useEPCShiftLight);
  DEBUG_PRINTLN(shiftLimit);
#endif
}
