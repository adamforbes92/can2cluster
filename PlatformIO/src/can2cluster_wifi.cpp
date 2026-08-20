#include "can2cluster_wifi.h"
#include "can2cluster_savvycan.h"
#include "can2cluster_gps.h"
#include "can2cluster_i2c.h"
#include "power_manager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <esp_chip_info.h>

AsyncWebServer server(80);

static bool handleTestSpeedRpmControl(const String &key, JsonVariant value)
{
  if (key == "testSpeedo")
  {
    testSpeedo = value.as<bool>();
    return true;
  }
  if (key == "tempSpeed")
  {
    tempSpeed = value.as<int>();
    return true;
  }
  if (key == "testRPM")
  {
    testRPM = value.as<bool>();
    return true;
  }
  if (key == "tempRPM")
  {
    tempRPM = value.as<int>();
    return true;
  }

  return false;
}

static bool handleTestOutputControl(const String &key, JsonVariant value)
{
  if (key == "testReverse")
  {
    testReverse = value.as<bool>();
    return true;
  }
  if (key == "testEML")
  {
    testEML = value.as<bool>();
    return true;
  }
  if (key == "testEPC")
  {
    testEPC = value.as<bool>();
    return true;
  }

  return false;
}

static uint32_t parseCanId(const String &raw, uint32_t defaultVal)
{
  String value = raw;
  value.trim();
  if (value.startsWith("0x") || value.startsWith("0X")) {
    value = value.substring(2);
  }

  char *endPtr = nullptr;
  unsigned long parsed = strtoul(value.c_str(), &endPtr, 16);
  if (endPtr == value.c_str() || *endPtr != '\0') {
    return defaultVal;
  }

  return static_cast<uint32_t>(parsed) & 0x7FF;
}

static bool handleBroadcastSpeedControl(const String &key, JsonVariant value)
{
  if (key == "broadcastSpeedEnabled") {
    broadcastSpeedEnabled = value.as<bool>();
    return true;
  }
  if (key == "broadcastSpeedID") {
    if (value.is<const char*>()) {
      broadcastSpeedID = parseCanId(String(value.as<const char*>()), broadcastSpeedID);
    } else {
      broadcastSpeedID = value.as<uint32_t>() & 0x7FF;
    }
    return true;
  }
  if (key == "broadcastSpeedDLC") {
    broadcastSpeedDLC = constrain(value.as<int>(), 0, 8);
    return true;
  }
  if (key == "broadcastSpeedLowByte") {
    broadcastSpeedLowByte = constrain(value.as<int>(), 0, 7);
    return true;
  }
  if (key == "broadcastSpeedHighByte") {
    broadcastSpeedHighByte = constrain(value.as<int>(), 0, 7);
    return true;
  }
  if (key == "broadcastSpeedLittleEndian") {
    broadcastSpeedLittleEndian = value.as<bool>();
    return true;
  }
  if (key == "broadcastSpeedScale") {
    broadcastSpeedScale = value.as<float>();
    return true;
  }
  if (key == "broadcastSpeedOffset") {
    broadcastSpeedOffset = value.as<int>();
    return true;
  }

  for (uint8_t i = 0; i < 8; i++) {
    String dataKey = "broadcastSpeedData" + String(i);
    if (key == dataKey) {
      broadcastSpeedData[i] = constrain(value.as<int>(), 0, 255);
      return true;
    }
  }

  return false;
}

static bool handleAftermarketControl(const String &key, JsonVariant value)
{
  if (key == "aftermarketSpeedID") {
    if (value.is<const char*>()) {
      aftermarketSpeedID = parseCanId(String(value.as<const char*>()), aftermarketSpeedID);
    } else {
      aftermarketSpeedID = value.as<uint32_t>() & 0x7FF;
    }
    return true;
  }
  if (key == "aftermarketSpeedLowByte") {
    aftermarketSpeedLowByte = constrain(value.as<int>(), 0, 7);
    return true;
  }
  if (key == "aftermarketSpeedHighByte") {
    aftermarketSpeedHighByte = constrain(value.as<int>(), 0, 7);
    return true;
  }
  if (key == "aftermarketSpeedLittleEndian") {
    aftermarketSpeedLittleEndian = value.as<bool>();
    return true;
  }
  if (key == "aftermarketSpeedScale") {
    aftermarketSpeedScale = value.as<float>();
    return true;
  }
  if (key == "aftermarketSpeedOffset") {
    aftermarketSpeedOffset = value.as<int>();
    return true;
  }
  return false;
}

static bool handleDsgCalcControl(const String &key, JsonVariant value)
{
  for (uint8_t i = 1; i <= 6; i++) {
    if (key == "dsgRatio" + String(i)) {
      float v = value.as<float>();
      if (v > 0.0f) dsgGearRatio[i] = v;
      return true;
    }
  }
  if (key == "dsgFinal14") {
    float v = value.as<float>();
    if (v > 0.0f) dsgFinalDrive14 = v;
    return true;
  }
  if (key == "dsgFinal56") {
    float v = value.as<float>();
    if (v > 0.0f) dsgFinalDrive56 = v;
    return true;
  }
  if (key == "dsgTireCirc") {
    float v = value.as<float>();
    if (v > 0.0f) dsgTireCirc = v;
    return true;
  }
  return false;
}

static bool handleTestAction(const String &action)
{
  if (action == "needleSweep")
  {
    tempNeedleSweep = true;
    return true;
  }
  if (action == "testShiftLight")
  {
    tempShiftLight = true;
    return true;
  }

  return false;
}

// Clears any EML/EPC light feature that clashes with the pin the coolant gauge
// now owns, so a single pin is never driven for two purposes at once.
static void applyCoolantExclusivity()
{
  if (coolantOutput == 1) // EML pin taken by coolant
  {
    useEMLShiftLight = false;
    testEML = false;
    if (dsgParkMode == "EML") dsgParkMode = "None";
  }
  else if (coolantOutput == 2) // EPC pin taken by coolant
  {
    useEPCShiftLight = false;
    testEPC = false;
    if (dsgParkMode == "EPC") dsgParkMode = "None";
  }
}

// Add or update a calibration point (temperature -> current jog duty), keeping
// the table sorted ascending by temperature. Returns false if the table is full.
static bool coolantAddPoint(int16_t tempC, uint16_t duty)
{
  for (uint8_t i = 0; i < coolantCalCount; i++)
  {
    if (coolantCalTemp[i] == tempC)
    {
      coolantCalDuty[i] = duty;
      return true;
    }
  }
  if (coolantCalCount >= COOLANT_CAL_MAX)
    return false;

  uint8_t pos = 0;
  while (pos < coolantCalCount && coolantCalTemp[pos] < tempC)
    pos++;
  for (uint8_t i = coolantCalCount; i > pos; i--)
  {
    coolantCalTemp[i] = coolantCalTemp[i - 1];
    coolantCalDuty[i] = coolantCalDuty[i - 1];
  }
  coolantCalTemp[pos] = tempC;
  coolantCalDuty[pos] = duty;
  coolantCalCount++;
  return true;
}

static void coolantDeletePoint(uint8_t index)
{
  if (index >= coolantCalCount)
    return;
  for (uint8_t i = index; i + 1 < coolantCalCount; i++)
  {
    coolantCalTemp[i] = coolantCalTemp[i + 1];
    coolantCalDuty[i] = coolantCalDuty[i + 1];
  }
  coolantCalCount--;
}

static void sendCoolantCalState(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  doc["output"] = coolantOutput == 1 ? "EML" : (coolantOutput == 2 ? "EPC" : "Off");
  doc["freq"] = coolantPwmFreq;
  doc["calMode"] = coolantCalMode;
  doc["duty"] = coolantCalDutyNow;
  doc["maxDuty"] = coolantMaxOut();
  doc["appliedDuty"] = coolantAppliedDuty;
  doc["temp"] = vehicleCoolantTemp;
  doc["count"] = coolantCalCount;
  JsonArray pts = doc["points"].to<JsonArray>();
  for (uint8_t i = 0; i < coolantCalCount; i++)
  {
    JsonObject p = pts.add<JsonObject>();
    p["temp"] = coolantCalTemp[i];
    p["duty"] = coolantCalDuty[i];
  }
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  serializeJson(doc, *response);
  request->send(response);
}

void setupWebRoutes()
{
  // Serve static files from LittleFS.
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    JsonDocument doc;
    doc["FW_VERSION"] = FW_VERSION;

    // Settings
    doc["hasNeedleSweep"] = hasNeedleSweep;
    doc["sweepSpeed"] = sweepSpeed;
    doc["stepRPM"] = stepRPM;
    doc["stepSpeed"] = stepSpeed;
    doc["shiftLight"] = (useEMLShiftLight && useEPCShiftLight) ? "Both" : (useEMLShiftLight ? "EML" : (useEPCShiftLight ? "EPC" : "None"));
    doc["shiftLimit"] = shiftLimit;
    doc["shiftFlashes"] = shiftFlashes;
    doc["coilType"] = coilType;
    doc["useCoil"] = coilType;
    doc["useMPH"] = useMPH;
    doc["diagTest"] = diagTest;
    doc["analyzerMode"] = analyzerMode;
    doc["analyzerSerial"] = analyzerSerial;
    doc["dsgParkMode"] = dsgParkMode;

    // Coolant gauge output
    doc["coolantOutput"] = coolantOutput == 1 ? "EML" : (coolantOutput == 2 ? "EPC" : "Off");
    doc["coolantPwmFreq"] = coolantPwmFreq;
    doc["coolantWarnTemp"] = coolantWarnTemp;
    
    // Advanced controls
    doc["testRPM"] = testRPM;
    doc["tempRPM"] = tempRPM;
    doc["testSpeedo"] = testSpeedo;
    doc["tempSpeed"] = tempSpeed;
    doc["testReverse"] = testReverse;
    doc["testEML"] = testEML;
    doc["testEPC"] = testEPC;
    doc["broadcastSpeedEnabled"] = broadcastSpeedEnabled;
    doc["broadcastSpeedID"] = broadcastSpeedID;
    doc["broadcastSpeedDLC"] = broadcastSpeedDLC;
    doc["broadcastSpeedLowByte"] = broadcastSpeedLowByte;
    doc["broadcastSpeedHighByte"] = broadcastSpeedHighByte;
    doc["broadcastSpeedLittleEndian"] = broadcastSpeedLittleEndian;
    doc["broadcastSpeedScale"] = broadcastSpeedScale;
    doc["broadcastSpeedOffset"] = broadcastSpeedOffset;
    for (uint8_t i = 0; i < 8; i++) {
      String dataKey = "broadcastSpeedData" + String(i);
      doc[dataKey] = broadcastSpeedData[i];
    }

    doc["aftermarketSpeedID"] = aftermarketSpeedID;
    doc["aftermarketSpeedLowByte"] = aftermarketSpeedLowByte;
    doc["aftermarketSpeedHighByte"] = aftermarketSpeedHighByte;
    doc["aftermarketSpeedLittleEndian"] = aftermarketSpeedLittleEndian;
    doc["aftermarketSpeedScale"] = aftermarketSpeedScale;
    doc["aftermarketSpeedOffset"] = aftermarketSpeedOffset;

    // DSG speed calculation (RPM & gear -> speed)
    for (uint8_t i = 1; i <= 6; i++) {
      String ratioKey = "dsgRatio" + String(i);
      doc[ratioKey] = dsgGearRatio[i];
    }
    doc["dsgFinal14"] = dsgFinalDrive14;
    doc["dsgFinal56"] = dsgFinalDrive56;
    doc["dsgTireCirc"] = dsgTireCirc;

    // Speed type selection
    if (useHall) doc["speedType"] = "Hall";
    else if (useVR) doc["speedType"] = "VR";
    else if (useECU) doc["speedType"] = "ECU";
    else if (useABS) doc["speedType"] = "ABS";
    else if (useDSG) doc["speedType"] = "DSG";
    else if (useTP20) doc["speedType"] = "TP2.0";
    else if (useUDS) doc["speedType"] = "UDS";
    else if (useGPS) doc["speedType"] = "GPS";
    else if (useAftermarket) doc["speedType"] = "Custom CAN";
    else doc["speedType"] = "Hall";

    doc["rpmType"] = useHallRPM ? "Hall" : "CAN";
    doc["clusterFrequencyLimit"] = maxRPM;
    doc["clusterRPMLimit"] = clusterRPMLimit;
    doc["maxFreqHall"] = maxFreqHall;
    doc["maxFreqVR"] = maxFreqVR;
    doc["gpsUpdateRateHz"] = gpsUpdateRateHz;
    
    // Diagnostic query status
    doc["autoDiagQuery"] = autoDiagQuery;

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response); });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    JsonDocument doc;
    doc["hasCAN"] = hasCAN;
    doc["hasGPS"] = hasGPS;
    doc["gpsUnavailable"] = gpsUnavailable;
    doc["gpsSatellites"] = gpsSatellites;
    doc["gpsFrequency"] = getGPSUpdateFrequency();
    doc["gpsAutoApplySecs"] = gpsAutoApplySecondsRemaining();
    doc["hallSpeed"] = hallSpeed;
    doc["vrSpeed"] = vrSpeed;
    doc["ecuSpeed"] = ecuSpeed;
    doc["absSpeed"] = absSpeed;
    doc["dsgSpeed"] = dsgSpeed;
    doc["udsSpeed"] = udsSpeed;    // UDS speed (ISO 14229)
    doc["tp20Speed"] = tp20Speed;   // TP2.0 DSG speed
    doc["gpsSpeed"] = gpsSpeed;
    doc["vehicleRPM"] = vehicleRPM;
    doc["canRPM"] = vehicleRPMCAN;
    doc["hallRPM"] = dutyCycleMotor;
    doc["frequencyRPM"] = frequencyRPM;  // RPM Final
    doc["vehicleSpeed"] = vehicleSpeed;
    doc["frequencySpeed"] = frequencySpeed;  // Speed Final
    doc["vehicleEML"] = vehicleEML;
    doc["vehicleEPC"] = vehicleEPC;
    doc["vehicleReverse"] = vehicleReverse;
    doc["vehiclePark"] = vehiclePark;
    doc["paddleUp"] = boolPadUp;
    doc["paddleDown"] = boolPadDown;
    doc["testRPM"] = testRPM;
    doc["tempRPM"] = tempRPM;
    doc["testSpeedo"] = testSpeedo;
    doc["tempSpeed"] = tempSpeed;
    doc["testReverse"] = testReverse;
    doc["testEML"] = testEML;
    doc["testEPC"] = testEPC;
    doc["tempNeedleSweep"] = tempNeedleSweep;
    doc["broadcastSpeedEnabled"] = broadcastSpeedEnabled;
    doc["broadcastSpeedValue"] = broadcastSpeedValue;
    doc["aftermarketSpeed"] = aftermarketSpeed;
    doc["useMPH"] = useMPH;
    doc["diagTest"] = diagTest;
    doc["analyzerMode"] = analyzerMode;
    doc["analyzerSerial"] = analyzerSerial;
    doc["freeHeap"] = ESP.getFreeHeap();

    // Coolant gauge output
    doc["vehicleCoolantTemp"] = vehicleCoolantTemp;
    doc["coolantDuty"] = coolantAppliedDuty;
    doc["coolantCalMode"] = coolantCalMode;
    doc["coolantOutputActive"] = (coolantOutput != 0);

    // Board revision + I2C peripheral diagnostics
    JsonObject i2c = doc["i2c"].to<JsonObject>();
    i2c["board"] = isNewBoard ? "new" : "old";
    i2c["newBoard"] = isNewBoard;
    i2c["mcp4725"] = i2cMcpPresent;
    i2c["tca9554"] = i2cTcaPresent;
    i2c["tcaInputs"] = tcaInputShadow;
    i2c["tcaIntCount"] = (uint32_t)tcaIntCount;
    i2c["coolantDac"] = coolantAppliedDuty;
    
    /*
     Settings
    doc["hasNeedleSweep"] = hasNeedleSweep;
    doc["sweepSpeed"] = sweepSpeed;
    doc["stepRPM"] = stepRPM;
    doc["stepSpeed"] = stepSpeed;
    doc["shiftLight"] = (useEMLShiftLight && useEPCShiftLight) ? "Both" : (useEMLShiftLight ? "EML" : (useEPCShiftLight ? "EPC" : "None"));
    doc["shiftLimit"] = shiftLimit;
    doc["shiftFlashes"] = shiftFlashes;
    doc["coilType"] = coilType;
    doc["dsgParkMode"] = dsgParkMode;
    
    // Advanced controls
    doc["testRPM"] = testRPM;
    doc["tempRPM"] = tempRPM;
    doc["testSpeedo"] = testSpeedo;
    doc["tempSpeed"] = tempSpeed;
    doc["testReverse"] = testReverse;
    doc["testEML"] = testEML;
    doc["testEPC"] = testEPC;
    
    // Speed type selection
    if (useHall) doc["speedType"] = "Hall";
    else if (useVR) doc["speedType"] = "VR";
    else if (useECU) doc["speedType"] = "ECU";
    else if (useABS) doc["speedType"] = "ABS";
    else if (useDSG) doc["speedType"] = "DSG";
    else if (useTP20) doc["speedType"] = "TP2.0";
    else if (useUDS) doc["speedType"] = "UDS";
    else if (useGPS) doc["speedType"] = "GPS";
    else doc["speedType"] = "Hall";
    
    // Diagnostic query status
    doc["autoDiagQuery"] = autoDiagQuery;
    */
    
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response); });

  server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (const char*)data, len);
    if (err) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");
      return;
    }
    
    String key = doc["key"];
    JsonVariant value = doc["value"];

    if (key.length() == 0) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"missing_key\"}");
      return;
    }

    if (handleTestSpeedRpmControl(key, value)) {
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    if (handleTestOutputControl(key, value)) {
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    if (handleBroadcastSpeedControl(key, value)) {
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    if (handleAftermarketControl(key, value)) {
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    if (handleDsgCalcControl(key, value)) {
      request->send(200, "application/json", "{\"ok\":true}");
      return;
    }

    bool settingApplied = false;

    if (key == "hasNeedleSweep") hasNeedleSweep = value.as<bool>();
    if (key == "hasNeedleSweep") settingApplied = true;
    if (key == "sweepSpeed") {
      sweepSpeed = value.as<int>();
      settingApplied = true;
    }
    if (key == "stepRPM") {
      stepRPM = value.as<int>();
      settingApplied = true;
    }
    if (key == "stepSpeed") {
      stepSpeed = value.as<int>();
      settingApplied = true;
    }
    if (key == "coilType" || key == "useCoil") {
      coilType = value.as<bool>();
      settingApplied = true;
    }
    if (key == "useMPH") {
      useMPH = value.as<bool>();
      settingApplied = true;
    }
    if (key == "diagTest") {
      diagTest = value.as<bool>();
      settingApplied = true;
    }
    if (key == "analyzerMode") {
      analyzerMode = value.as<bool>();
      if (analyzerMode) analyzerSerial = false;  // mutually exclusive
      setAnalyzerMode(analyzerMode);
      settingApplied = true;
    }
    if (key == "analyzerSerial") {
      analyzerSerial = value.as<bool>();
      if (analyzerSerial) analyzerMode = false;  // mutually exclusive
      setAnalyzerSerialMode(analyzerSerial);
      settingApplied = true;
    }
    if (key == "dsgParkMode") {
      String mode = value.as<const char*>();
      dsgParkMode = mode;
      applyCoolantExclusivity();
      settingApplied = true;
    }
    
    if (key == "shiftLight") {
      String mode = value.as<const char*>();
      useEMLShiftLight = (mode == "EML" || mode == "Both");
      useEPCShiftLight = (mode == "EPC" || mode == "Both");
      applyCoolantExclusivity();
      settingApplied = true;
    }

    if (key == "coolantOutput") {
      String mode = value.as<const char*>();
      coolantOutput = (mode == "EML") ? 1 : (mode == "EPC") ? 2 : 0;
      applyCoolantExclusivity();
      settingApplied = true;
    }
    if (key == "coolantPwmFreq") {
      coolantPwmFreq = constrain(value.as<int>(), 10, COOLANT_PWM_FREQ_MAX);
      settingApplied = true;
    }
    if (key == "coolantWarnTemp") {
      coolantWarnTemp = constrain(value.as<int>(), 0, 140);
      settingApplied = true;
    }
    
    if (key == "shiftLimit") {
      shiftLimit = value.as<int>();
      settingApplied = true;
    }
    if (key == "shiftFlashes") {
      shiftFlashes = value.as<int>();
      settingApplied = true;
    }
    
    if (key == "speedType") {
      String st = value.as<const char*>();
      useHall       = st == "Hall";
      useVR         = st == "VR";
      useECU        = st == "ECU";
      useDSG        = st == "DSG";
      useABS        = st == "ABS";
      useGPS        = st == "GPS";
      useTP20       = st == "TP2.0";
      useUDS        = st == "UDS";
      useAftermarket = st == "Custom CAN";
      // Activate live diagnostics when TP2.0 or UDS is selected
      autoDiagQuery = useTP20 || useUDS;
      settingApplied = true;
    }

    if (key == "rpmType") {
      String rt = value.as<const char*>();
      useHallRPM = rt == "Hall";
      settingApplied = true;
    }

    if (key == "clusterFrequencyLimit") {
      maxRPM = value.as<int>();
      settingApplied = true;
    }

    if (key == "maxFreqHall") {
      maxFreqHall = value.as<int>();
      settingApplied = true;
    }

    if (key == "maxFreqVR") {
      maxFreqVR = value.as<int>();
      settingApplied = true;
    }

    if (key == "clusterRPMLimit") {
      clusterRPMLimit = value.as<int>();
      settingApplied = true;
    }

    if (!settingApplied) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"unknown_key\"}");
      return;
    }

    request->send(200, "application/json", "{\"ok\":true}"); });

  server.on("/api/action", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    deserializeJson(doc, (const char*)data, len);
    String action = doc["action"];

    handleTestAction(action);
    
    request->send(200, "application/json", "{\"ok\":true}"); });

  // Coolant calibration builder: report live state, or apply a builder op.
  server.on("/api/coolantcal", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendCoolantCalState(request); });

  server.on("/api/coolantcal", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    if (index + len != total) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, (const char*)data, len);
    if (err) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_json\"}");
      return;
    }

    String op = doc["op"] | "";

    if (op == "enter") {
      coolantCalMode = true;
    } else if (op == "exit") {
      coolantCalMode = false;
    } else if (op == "jog") {
      long d = (long)coolantCalDutyNow + (int)(doc["delta"] | 0);
      coolantCalDutyNow = (uint16_t)constrain(d, 0L, (long)coolantMaxOut());
      coolantCalMode = true;
    } else if (op == "setDuty") {
      coolantCalDutyNow = (uint16_t)constrain((int)(doc["duty"] | 0), 0, (int)coolantMaxOut());
      coolantCalMode = true;
    } else if (op == "addPoint") {
      int16_t t = (int16_t)(doc["temp"] | 0);
      if (!coolantAddPoint(t, coolantCalDutyNow)) {
        request->send(400, "application/json", "{\"ok\":false,\"error\":\"table_full\"}");
        return;
      }
    } else if (op == "deletePoint") {
      coolantDeletePoint((uint8_t)(doc["index"] | 0));
    } else if (op == "clearPoints") {
      coolantCalCount = 0;
    } else {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"unknown_op\"}");
      return;
    }

    sendCoolantCalState(request); });

      server.on("/api/gpsRate", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                {
        if (index + len != total) return;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, (const char*)data, len);
        if (err) {
          request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
          return;
        }

        uint8_t rate = doc["rate"] | 0;
        String resp;
        bool ok = setGPSUpdateRate(rate, resp);

        JsonDocument out;
        out["success"] = ok;
        out["message"] = resp;
        String response;
        serializeJson(out, response);
        request->send(ok ? 200 : 400, "application/json", response);
      });

  // OTA Update API endpoints
  server.on("/api/ota/info", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    JsonDocument doc;
    
    // Get chip info
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    // Build board string
    String board = "ESP32";
    board += " (";
    board += chip_info.cores;
    board += " cores";
    if (chip_info.revision > 0) {
      board += " Rev.";
      board += chip_info.revision;
    }
    board += ")";
    
    // Hardware info
    String hardware = "ESP32 ";
    hardware += (chip_info.revision > 0 ? "Revision " : "");
    hardware += chip_info.revision;
    
    doc["board"] = board;
    doc["hardware"] = hardware;
    doc["version"] = FW_VERSION;
    
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response); });

  server.on("/api/ota", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool success = !Update.hasError();
      request->send(success ? 200 : 500, "application/json",
                    success ? "{\"success\":true}" : "{\"success\":false}");
      if (success) {
        xTaskCreate([](void*) {
          vTaskDelay(pdMS_TO_TICKS(1500));
          ESP.restart();
          vTaskDelete(nullptr);
        }, "ota_reboot", 2048, nullptr, 1, nullptr);
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0) {
        DEBUG_WIFI("Starting firmware OTA: %s", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      }
      if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (!Update.hasError()) {
          if (!Update.end(true)) {
            Update.printError(Serial);
          } else {
            DEBUG_WIFI("Firmware OTA complete");
          }
        }
      }
    });

  server.on("/api/ota/fs", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool success = !Update.hasError();
      request->send(success ? 200 : 500, "application/json",
                    success ? "{\"success\":true}" : "{\"success\":false}");
      if (success) {
        xTaskCreate([](void*) {
          vTaskDelay(pdMS_TO_TICKS(1500));
          ESP.restart();
          vTaskDelete(nullptr);
        }, "otafs_reboot", 2048, nullptr, 1, nullptr);
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0) {
        DEBUG_WIFI("Starting filesystem OTA: %s", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
          Update.printError(Serial);
        }
      }
      if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (!Update.hasError()) {
          if (!Update.end(true)) {
            Update.printError(Serial);
          } else {
            DEBUG_WIFI("Filesystem OTA complete");
          }
        }
      }
    });
}

void setupUI()
{
  if (!LittleFS.begin(true))
  {
    DEBUG_WIFI("LittleFS Mount Failed");
    return;
  }
  setupWebRoutes();
  server.begin();
  DEBUG_WIFI("Web server started");
}

void connectWifi()
{
  DEBUG_WIFI("Begin wifi...");
  WiFi.hostname(wifiHostName);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiHostName);
  WiFi.setSleep(false); // for the ESP32: turn off sleeping to increase UI responsivness (at the cost of power use)
  DEBUG_WIFI("WiFi access point started");
}

void disconnectWifi()
{
  DEBUG_WIFI("Number of connections: %d", WiFi.softAPgetStationNum());
  if (WiFi.softAPgetStationNum() == 0)
  {
    DEBUG_WIFI("No connections");
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
  }
}

// ----------------------------------------------------------------------------
// power_manager integration (universal reduced-power module)
// ----------------------------------------------------------------------------
// These override the weak hooks in power_manager. The device stays fully awake
// while ANY client is associated to the AP (web UI or SavvyCAN/TCP). Once the
// last client leaves, the manager's idle timer runs and then turns the radio
// off + drops the CPU clock. Power-cycle (ignition off/on) brings WiFi back.
bool powerIsBusy()
{
  return WiFi.softAPgetStationNum() > 0;
}

// ACTIVE -> REDUCED: close the web server cleanly before the radio drops. The
// SavvyCAN analyzer task self-heals (it watches for WIFI_OFF and closes its
// client), so it needs no explicit teardown here.
void powerOnEnterReduced()
{
  server.end();
}

// REDUCED -> ACTIVE: bring the AP and web server back. Routes are already
// registered and LittleFS already mounted from setupUI(), so we only restart
// the radio + listener (no need to re-run setupWebRoutes()).
void powerOnExitReduced()
{
  connectWifi();
  server.begin();
}
