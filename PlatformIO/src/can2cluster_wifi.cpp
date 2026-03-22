#include "can2cluster_wifi.h"
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

void setupWebRoutes()
{
  // Serve static files from LittleFS
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
    else if (useECU) doc["speedType"] = "ECU";
    else if (useABS) doc["speedType"] = "ABS";
    else if (useDSG) doc["speedType"] = "DSG";
    else if (useTPUDSDSG) doc["speedType"] = "TP2.0-DSG";
    else if (useGPS) doc["speedType"] = "GPS";
    else doc["speedType"] = "Hall";

    doc["rpmType"] = useHallRPM ? "Hall" : "CAN";
    doc["clusterFrequencyLimit"] = maxRPM;
    doc["clusterRPMLimit"] = clusterRPMLimit;
    
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
    doc["gpsSatellites"] = gpsSatellites;
    doc["gpsTaskSuspended"] = gpsTaskSuspended;
    doc["hallSpeed"] = hallSpeed;
    doc["ecuSpeed"] = ecuSpeed;
    doc["absSpeed"] = absSpeed;
    doc["dsgSpeed"] = dsgSpeed;
    doc["udsSpeed"] = dsgUDSSpeed;  // TP2.0/UDS DSG speed
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
    doc["freeHeap"] = ESP.getFreeHeap();
    
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
    else if (useECU) doc["speedType"] = "ECU";
    else if (useABS) doc["speedType"] = "ABS";
    else if (useDSG) doc["speedType"] = "DSG";
    else if (useTPUDSDSG) doc["speedType"] = "TP2.0-DSG";
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
    if (key == "dsgParkMode") {
      String mode = value.as<const char*>();
      dsgParkMode = mode;
      settingApplied = true;
    }
    
    if (key == "shiftLight") {
      String mode = value.as<const char*>();
      useEMLShiftLight = (mode == "EML" || mode == "Both");
      useEPCShiftLight = (mode == "EPC" || mode == "Both");
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
      useHall = st == "Hall";
      useECU = st == "ECU";
      useDSG = st == "DSG";
      useABS = st == "ABS";
      useGPS = st == "GPS";
      // TP/UDS DSG handled separately - still uses DSG speed value but from UDS protocol
      useTPUDSDSG = st == "TP2.0-DSG";
      // Only activate UDS/TP diagnostics when TP/UDS DSG is selected
      autoDiagQuery = useTPUDSDSG;
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

    if (key == "clusterRPMLimit") {
      clusterRPMLimit = value.as<int>();
      settingApplied = true;
    }

    if (!settingApplied) {
      request->send(400, "application/json", "{\"ok\":false,\"error\":\"unknown_key\"}");
      return;
    }

    eepDirty = true;  // settings changed — schedule a flash write
    request->send(200, "application/json", "{\"ok\":true}"); });

  server.on("/api/action", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    JsonDocument doc;
    deserializeJson(doc, (const char*)data, len);
    String action = doc["action"];

    handleTestAction(action);
    
    request->send(200, "application/json", "{\"ok\":true}"); });

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

  server.on("/api/ota/upload", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    request->send(200, "application/json", "{\"message\":\"Update completed. Device will reboot...\"}"); }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    // Handle file upload
    if (index == 0) {
      DEBUG_WIFI("Starting OTA update, filename: %s", filename.c_str());
      
      // Start the update
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
        DEBUG_WIFI("OTA Update.begin() failed");
        return;
      }
    }
    
    // Write data to update partition
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
      DEBUG_WIFI("OTA Update.write() failed");
      return;
    }
    
    if (final) {
      if (Update.end(true)) {
        DEBUG_WIFI("OTA Update successful, rebooting...");
        // Schedule reboot after a short delay to allow response to be sent
        delay(500);
        ESP.restart();
      } else {
        Update.printError(Serial);
        DEBUG_WIFI("OTA Update.end() failed");
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
  DEBUG("Number of connections: %d", WiFi.softAPgetStationNum());
  if (WiFi.softAPgetStationNum() == 0)
  {
    DEBUG("No connections");
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
  }
}
