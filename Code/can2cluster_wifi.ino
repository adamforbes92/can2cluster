void setupUI() {
  ESPUI.setVerbosity(Verbosity::Verbose);  // turn off verbose debugging (Verbose for ON; Quiet for OFF)
  ESPUI.sliderContinuous = true;           // update slider valves constantly disabled.  No need and can cause crashes

  // create basic tab
  auto tabBasic = ESPUI.addControl(Tab, "", "Basic");
  ESPUI.addControl(Separator, "Needle Sweep", "", Dark, tabBasic);
  bool_NeedleSweep = ESPUI.addControl(Switcher, "Needle Sweep", String(hasNeedleSweep), Dark, tabBasic, generalCallback);
  int16_sweepSpeed = ESPUI.addControl(Slider, "Rate of Change (ms)", String(sweepSpeed), Dark, tabBasic, generalCallback);
  ESPUI.addControl(Max, "", "50", Dark, int16_sweepSpeed);

  int16_stepRPM = ESPUI.addControl(Slider, "Rate RPM", String(stepRPM), Dark, tabBasic, generalCallback);
  ESPUI.addControl(Max, "", "100", Dark, int16_stepRPM);

  int16_stepSpeed = ESPUI.addControl(Slider, "Rate Speed", String(stepSpeed), Dark, tabBasic, generalCallback);
  ESPUI.addControl(Max, "", "100", Dark, int16_stepSpeed);

  ESPUI.addControl(Button, "Test Needle Sweep", "Test", Dark, tabBasic, extendedCallback, (void *)11);

  // create advanced tab
  auto tabAdvanced = ESPUI.addControl(Tab, "", "Advanced");
  ESPUI.addControl(Separator, "Testing", "", Dark, tabAdvanced);
  bool_testSpeedo = ESPUI.addControl(Switcher, "Test Speedo", "", Dark, tabAdvanced, generalCallback);
  int16_tempSpeed = ESPUI.addControl(Slider, "Go to Speed", String(tempSpeed), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "200", Dark, int16_tempSpeed);

  ESPUI.addControl(Separator, "RPM Output", "", Dark, tabAdvanced);
  bool_coilType = ESPUI.addControl(Switcher, "Coil Type", String(coilType), Dark, tabAdvanced, generalCallback);

  bool_testRPM = ESPUI.addControl(Switcher, "Test RPM", "", Dark, tabAdvanced, generalCallback);
  int16_tempRPM = ESPUI.addControl(Slider, "Go to RPM", String(tempRPM), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "8000", Dark, int16_tempRPM);

  ESPUI.addControl(Separator, "Shift Light", "", Dark, tabAdvanced);
  int16_shiftLight = ESPUI.addControl(Select, "Output Type", "", Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Option, "None", "None", Dark, int16_shiftLight);
  ESPUI.addControl(Option, "EML Output", "EML", Dark, int16_shiftLight);
  ESPUI.addControl(Option, "EPC Output", "EPC", Dark, int16_shiftLight);
  ESPUI.addControl(Option, "EML & EPC Output", "Both", Dark, int16_shiftLight);

  int16_shiftRPM = ESPUI.addControl(Slider, "Set Shift RPM", String(shiftLimit), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "8000", Dark, int16_shiftRPM);
  int16_shiftFlashes = ESPUI.addControl(Slider, "Flashes", String(shiftFlashes), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "5", Dark, int16_shiftFlashes);
  ESPUI.addControl(Button, "Test Shift Light", "Test", Dark, tabAdvanced, extendedCallback, (void *)14);

  ESPUI.addControl(Separator, "Park Lock", "", Dark, tabAdvanced);
  int16_parkLock = ESPUI.addControl(Select, "Output Type", "", Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Option, "None", "None", Dark, int16_parkLock);
  ESPUI.addControl(Option, "EML Output", "EML", Dark, int16_parkLock);
  ESPUI.addControl(Option, "EPC Output", "EPC", Dark, int16_parkLock);

  ESPUI.addControl(Separator, "Speed Limits:", "", Dark, tabAdvanced);
  int16_maxSpeed = ESPUI.addControl(Slider, "Maximum Speed", String(maxSpeed), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "400", Dark, int16_maxSpeed);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvanced, extendedCallback, (void *)12);

  ESPUI.addControl(Separator, "Hall Incoming Freq:", "", Dark, tabAdvanced);
  int16_maxHall = ESPUI.addControl(Slider, "Maximum Hall (Speed)", String(maxFreqHall), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "400", Dark, int16_maxHall);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvanced, extendedCallback, (void *)13);

  ESPUI.addControl(Separator, "Hall Incoming Freq:", "", Dark, tabAdvanced);
  int16_maxHallRPM = ESPUI.addControl(Slider, "Maximum Hall (RPM)", String(maxFreqRPM), Dark, tabAdvanced, generalCallback);
  ESPUI.addControl(Max, "", "400", Dark, int16_maxHallRPM);
  ESPUI.addControl(Button, "Reset", "Reset", Dark, tabAdvanced, extendedCallback, (void *)14);

  // create IO tab
  auto tabAdvancedCAN = ESPUI.addControl(Tab, "", "IO");
  ESPUI.addControl(Separator, "Incoming Data", "", Dark, tabAdvancedCAN);
  ESPUI.addControl(Separator, "CAN Available:", "", Dark, tabAdvancedCAN);
  label_hasCAN = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "GPS Available:", "", Dark, tabAdvancedCAN);
  label_hasGPS = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "Speed Input:", "", Dark, tabAdvancedCAN);
  int16_speedType = ESPUI.addControl(Select, "Speed Type", "", Dark, tabAdvancedCAN, generalCallback);
  ESPUI.addControl(Option, "Hall Sensor", "Hall", Dark, int16_speedType);
  ESPUI.addControl(Option, "ECU (via. CAN)", "ECU", Dark, int16_speedType);
  ESPUI.addControl(Option, "ABS (via. CAN)", "ABS", Dark, int16_speedType);
  ESPUI.addControl(Option, "DSG (via. CAN)", "DSG", Dark, int16_speedType);
  ESPUI.addControl(Option, "GPS Module", "GPS", Dark, int16_speedType);

  ESPUI.addControl(Separator, "Incoming Speed:", "", Dark, tabAdvancedCAN);
  label_speedHall = ESPUI.addControl(Label, "Hall Speed:", "0", Dark, tabAdvancedCAN, generalCallback);
  label_speedECU = ESPUI.addControl(Label, "ECU Speed:", "0", Dark, tabAdvancedCAN, generalCallback);
  label_speedDSG = ESPUI.addControl(Label, "DSG Speed:", "0", Dark, tabAdvancedCAN, generalCallback);
  label_speedABS = ESPUI.addControl(Label, "ABS Speed:", "0", Dark, tabAdvancedCAN, generalCallback);
  label_speedGPS = ESPUI.addControl(Label, "GPS Speed:", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "Incoming RPM (CAN):", "", Dark, tabAdvancedCAN);
  label_RPMCAN = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "Incoming RPM (Hall):", "", Dark, tabAdvancedCAN);
  label_RPMHall = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "Incoming Paddles:", "", Dark, tabAdvancedCAN);
  label_paddleUp = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);
  label_paddleDown = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "EML:", "", Dark, tabAdvancedCAN);
  label_emlActive = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "EPC:", "", Dark, tabAdvancedCAN);
  label_epcActive = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "Reverse:", "", Dark, tabAdvancedCAN);
  label_reverseActive = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  ESPUI.addControl(Separator, "Park:", "", Dark, tabAdvancedCAN);
  label_parkActive = ESPUI.addControl(Label, "", "0", Dark, tabAdvancedCAN, generalCallback);

  auto tabTest = ESPUI.addControl(Tab, "", "Test");
  ESPUI.addControl(Separator, "Testing", "", Dark, tabTest);
  bool_testEML = ESPUI.addControl(Switcher, "Test EML", "", Dark, tabTest, generalCallback);
  bool_testEPC = ESPUI.addControl(Switcher, "Test EPC", "", Dark, tabTest, generalCallback);
  bool_testReverse = ESPUI.addControl(Switcher, "Test Reverse", "", Dark, tabTest, generalCallback);
  bool_testPark = ESPUI.addControl(Switcher, "Test Park Lock", "", Dark, tabTest, generalCallback);

  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(wifiHostName);
}

void updateCallback(Control *sender, int type) {
  updates = (sender->value.toInt() > 0);
}

void getTimeCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.updateTime(mainTime);
  }
}

void graphAddCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.addGraphPoint(graph, random(1, 50));
  }
}

void graphClearCallback(Control *sender, int type) {
  if (type == B_UP) {
    ESPUI.clearGraph(graph);
  }
}

void generalCallback(Control *sender, int type) {
#ifdef detailedDebugWiFi
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
#endif

  uint8_t tempID = int(sender->id);
  switch (tempID) {
    case 3:
      hasNeedleSweep = sender->value.toInt();
      break;
    case 4:
      sweepSpeed = sender->value.toInt();
      break;
    case 6:
      stepRPM = sender->value.toInt();
      break;
    case 8:
      stepSpeed = sender->value.toInt();
      break;

    case 13:
      testSpeedo = sender->value.toInt();
      break;
    case 14:
      tempSpeed = sender->value.toInt();
      break;
    case 17:
      coilType = sender->value.toInt();
      break;
    case 18:
      testRPM = sender->value.toInt();
      break;
    case 19:
      tempRPM = sender->value.toInt();
      break;
    case 22:
      if (sender->value == "None") {
        useEMLShiftLight = false;
        useEPCShiftLight = false;
      }
      if (sender->value == "EML") {
        useEMLShiftLight = true;
        useEPCShiftLight = false;
      }
      if (sender->value == "EPC") {
        useEMLShiftLight = false;
        useEPCShiftLight = true;
      }
      if (sender->value == "Both") {
        useEMLShiftLight = true;
        useEPCShiftLight = true;
      }
      break;
    case 27:
      shiftLimit = sender->value.toInt();
      break;
    case 29:
      shiftFlashes = sender->value.toInt();
      break;

    case 33:
      if (sender->value == "None") {
        useEMLPark = false;
        useEPCPark = false;
      }
      if (sender->value == "EML") {
        useEMLPark = true;
        useEPCPark = false;
      }
      if (sender->value == "EPC") {
        useEMLPark = false;
        useEPCPark = true;
      }
      break;
    case 38:
      maxSpeed = sender->value.toInt();
      break;
    case 42:
      maxFreqHall = sender->value.toInt();
      break;
    case 46:
      maxFreqRPM = sender->value.toInt();
      break;

    case 56:
      if (sender->value == "Hall") {
        useHall = true;
        useDSG = false;
        useECU = false;
        useABS = false;
        useGPS = false;
      }
      if (sender->value == "ECU") {
        useHall = false;
        useDSG = false;
        useECU = true;
        useABS = false;
        useGPS = false;
      }
      if (sender->value == "DSG") {
        useHall = false;
        useDSG = true;
        useECU = false;
        useABS = false;
        useGPS = false;
      }
      if (sender->value == "ABS") {
        useHall = false;
        useDSG = false;
        useECU = false;
        useABS = true;
        useGPS = false;
      }
      if (sender->value == "GPS") {
        useHall = false;
        useDSG = false;
        useECU = false;
        useABS = false;
        useGPS = true;
      }
      break;

    case 85:
      testEML = sender->value.toInt();
      break;
    case 86:
      testEPC = sender->value.toInt();
      break;
    case 87:
      testReverse = sender->value.toInt();
      break;
    case 88:
      testPark = sender->value.toInt();
      break;
  }
}

void extendedCallback(Control *sender, int type, void *param) {
#ifdef detailedDebugWiFi
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  Serial.print("param = ");
  Serial.println((long)param);
#endif

  uint8_t tempID = int(sender->id);
  switch (tempID) {
    case 10:
      if (type == B_UP) {
        tempNeedleSweep = true;
      }
      break;
    case 31:
      if (type == B_UP) {
        tempShiftLight = true;
      }
      break;
    case 40:
      if (type == B_UP) {
        maxSpeed = 200;
        ESPUI.updateSlider(int16_maxSpeed, maxSpeed);
      }
      break;
    case 44:
      if (type == B_UP) {
        maxFreqHall = 200;
        ESPUI.updateSlider(int16_maxHall, maxFreqHall);
      }
      break;
    case 48:
      if (type == B_UP) {
        maxFreqRPM = 230;
        ESPUI.updateSlider(int16_maxHallRPM, maxFreqRPM);
      }
      break;
  }
}

void selectCallback(Control *sender, int value) {
#ifdef detailedDebugWiFi
  Serial.print("Select: ID: ");
  Serial.print(sender->id);
  Serial.print(", Value: ");
  Serial.println(sender->value);
#endif
}

void setupWiFi() {
  WiFi.hostname(wifiHostName);

#if detailedDebugWiFi
  DEBUG("Beginning WiFi...");
  DEBUG("Creating Access Point...");
#endif

  //WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiHostName);
  WiFi.setSleep(false);  // for the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
}

void textCallback(Control *sender, int type) {
  //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

void randomString(char *buf, int len) {
  for (auto i = 0; i < len - 1; i++)
    buf[i] = random(0, 26) + 'A';
  buf[len - 1] = '\0';
}

void disconnectWifi() {
  DEBUG("Number of Connections: %d", WiFi.softAPgetStationNum());

  if (WiFi.softAPgetStationNum() == 0) {
    DEBUG("No connections, turning off");
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
  }
}

void updateLabels(void *arg) {
  while (1) {
    stackupdateLabels = uxTaskGetStackHighWaterMark(NULL);

    if ((millis() + 10 - lastCAN) < 500) {
      hasCAN = true;
    } else {
      hasCAN = false;
    }

    if (hasGPS) {
      char bufhasGPS[50];
      sprintf(bufhasGPS, "Yes (%d satellites)", gps.satellites.value());
      ESPUI.updateLabel(label_hasGPS, String(bufhasGPS));
    } else {
      char bufhasGPS[50];
      sprintf(bufhasGPS, "No (%d satellites)", gps.satellites.value());
      ESPUI.updateLabel(label_hasGPS, String(bufhasGPS));
    }

    hasCAN ? ESPUI.updateLabel(label_hasCAN, "Yes") : ESPUI.updateLabel(label_hasCAN, "No");
    boolPadUpWiFi ? ESPUI.updateLabel(label_paddleUp, "Paddle Up: Active") : ESPUI.updateLabel(label_paddleUp, "Paddle Up: Not Active");
    boolPadDownWiFi ? ESPUI.updateLabel(label_paddleDown, "Paddle Down: Active") : ESPUI.updateLabel(label_paddleDown, "Paddle Down: Not Active");


    if (!testPark) {
      testPark ? ESPUI.updateLabel(label_parkActive, "On") : ESPUI.updateLabel(label_parkActive, "Off");
    } else {
      ESPUI.updateLabel(label_parkActive, "On");
    }

    if (!testEML) {
      vehicleEML ? ESPUI.updateLabel(label_emlActive, "On") : ESPUI.updateLabel(label_emlActive, "Off");
    } else {
      ESPUI.updateLabel(label_emlActive, "On");
    }

    if (!testEPC) {
      vehicleEPC ? ESPUI.updateLabel(label_epcActive, "On") : ESPUI.updateLabel(label_epcActive, "Off");
    } else {
      ESPUI.updateLabel(label_epcActive, "On");
    }

    if (!testReverse) {
      vehicleReverse ? ESPUI.updateLabel(label_reverseActive, "On") : ESPUI.updateLabel(label_reverseActive, "Off");
    } else {
      ESPUI.updateLabel(label_reverseActive, "On");
    }

    ESPUI.updateLabel(label_speedHall, String(hallSpeed));
    ESPUI.updateLabel(label_speedGPS, String(gpsSpeed));
    ESPUI.updateLabel(label_speedDSG, String(dsgSpeed));
    ESPUI.updateLabel(label_speedABS, String(absSpeed));
    ESPUI.updateLabel(label_speedECU, String(ecuSpeed));
    ESPUI.updateLabel(label_RPMCAN, String(vehicleRPM));

    if (useHall) {
      ESPUI.updateSelect(int16_speedType, "Hall");
    }
    if (useABS) {
      ESPUI.updateSelect(int16_speedType, "ABS");
    }
    if (useDSG) {
      ESPUI.updateSelect(int16_speedType, "DSG");
    }
    if (useGPS) {
      ESPUI.updateSelect(int16_speedType, "GPS");
    }
    if (useECU) {
      ESPUI.updateSelect(int16_speedType, "ECU");
    }

    if (!useEMLShiftLight && !useEPCShiftLight) {
      ESPUI.updateSelect(int16_shiftLight, "None");
    }
    if (useEMLShiftLight && !useEPCShiftLight) {
      ESPUI.updateSelect(int16_shiftLight, "EML");
    }
    if (!useEMLShiftLight && useEPCShiftLight) {
      ESPUI.updateSelect(int16_shiftLight, "EPC");
    }
    if (useEMLShiftLight && useEPCShiftLight) {
      ESPUI.updateSelect(int16_shiftLight, "Both");
    }
    vTaskDelay(labelRefresh / portTICK_PERIOD_MS);
  }
}

void setupOTA() {
  updateServer.setup(ESPUI.WebServer(), "", "");

  updateServer.onUpdateBegin = [](const UpdateType type, int &result) {
//you can force abort the update like this if you need to:
//result = UpdateResult::UPDATE_ABORT;
#if detailedDebugWiFi
    DEBUG("Update started: %s", String(type));
#endif
  };
  updateServer.onUpdateEnd = [](const UpdateType type, int &result) {
#if detailedDebugWiFi
    DEBUG("Update finished: %s", String(type) + " result: " + String(result));
#endif
  };
}