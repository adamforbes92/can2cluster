#include "can2cluster_gps.h"

static unsigned long charsProcessedPrevious = 0;
static unsigned long passedChecksumPrevious = 0;
static unsigned long failedChecksumPrevious = 0;

static unsigned long gpsUpdateCount = 0;
static unsigned long gpsFreqWindowStart = 0;
static float gpsUpdateFrequency = 0.0f;
static const unsigned long GPS_COMMAND_DELAY_MS = 250;
static const unsigned long GPS_COMMAND_SETTLE_MS = 500;
static const unsigned long GPS_DEFAULT_BAUD = 9600UL;
static const unsigned long GPS_HIGH_RATE_BAUD = 38400UL;
static SemaphoreHandle_t gpsSerialMutex = nullptr;

// Tracks the actual baud rate the ESP serial is currently running at.
// Always GPS_DEFAULT_BAUD after boot — the GPS module resets to 9600 on power-up.
static unsigned long gpsCurrentSerialBaud = GPS_DEFAULT_BAUD;

// Millis of last GPS character received — used for timeout detection.
static unsigned long lastGPSData = 0;

// Satellite stability timer and rate-apply flag.
static unsigned long gpsSatStableStartMs = 0;
static bool gpsRateApplied = false;
static bool gpsPendingRateApply = false;
static const unsigned long GPS_SAT_STABLE_MS = 20000;

static const uint8_t UBX_1HZ[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x39};
static const uint8_t UBX_5HZ[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A};
static const uint8_t UBX_10HZ[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12};
static const uint8_t UBX_16HZ[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x01, 0x00, 0x54, 0xB6};
static const char PUBX_BAUD_9600[] = "PUBX,41,1,3,3,9600,0";
static const char PUBX_BAUD_38400[] = "PUBX,41,1,3,3,38400,0";
static const char PUBX_BAUD_57600[] = "PUBX,41,1,3,3,57600,0";
static const char PUBX_BAUD_115200[] = "PUBX,41,1,3,3,115200,0";

static SemaphoreHandle_t getGPSSerialMutex()
{
  if (gpsSerialMutex == nullptr)
  {
    gpsSerialMutex = xSemaphoreCreateMutex();
  }

  return gpsSerialMutex;
}

// Forward declarations — initGPS() uses these helpers before their definitions.
static bool applyGPSBaudRate(unsigned long baud, String &responseMsg);
static bool gpsProbeBaud(unsigned long baud);

// Startup: detect the baud the GPS module is actually running at, then lock the
// hardware UART onto it. The u-blox keeps its last-configured baud until it
// fully loses power, so after an ESP-only reset it may still be at the high-rate
// baud (38400). Unlike the old bit-banged SoftwareSerial (which passed bad bytes
// through), the hardware UART silently discards framing-error bytes, so opening
// at the wrong baud yields almost nothing. Probe the known bauds and lock on.
unsigned long initGPS()
{
  DEBUG_GPS("[GPS Init] Detecting GPS baud (default %lu, high-rate %lu)...", GPS_DEFAULT_BAUD, GPS_HIGH_RATE_BAUD);

  ss.setRxBufferSize(1024); // Generous RX buffer for 38400-baud bursts at 5 Hz.

  const unsigned long candidateBauds[] = {GPS_DEFAULT_BAUD, GPS_HIGH_RATE_BAUD};
  unsigned long detectedBaud = GPS_DEFAULT_BAUD;
  bool baudDetected = false;
  for (size_t i = 0; i < sizeof(candidateBauds) / sizeof(candidateBauds[0]); ++i)
  {
    if (gpsProbeBaud(candidateBauds[i]))
    {
      detectedBaud = candidateBauds[i];
      baudDetected = true;
      DEBUG_GPS("[GPS Init] Detected GPS at %lu baud.", detectedBaud);
      break;
    }
  }

  if (!baudDetected)
  {
    // No valid NMEA at any known baud (e.g. no module / no antenna yet). Fall
    // back to the default baud; the auto-rate logic will retry after lock.
    detectedBaud = GPS_DEFAULT_BAUD;
    ss.end();
    delay(20);
    ss.begin(detectedBaud, SERIAL_8N1, pinRX_GPS, pinTX_GPS);
    DEBUG_GPS("[GPS Init] No NMEA detected; defaulting to %lu baud.", detectedBaud);
  }

  charsProcessedPrevious = gps.charsProcessed(); // snapshot — TinyGPS counter never resets
  passedChecksumPrevious = gps.passedChecksum();
  failedChecksumPrevious = gps.failedChecksum();
  gpsUpdateCount = 0;
  gpsFreqWindowStart = 0;
  gpsUpdateFrequency = 0.0f;
  gpsSatStableStartMs = 0;
  gpsRateApplied = false;
  gpsCurrentSerialBaud = detectedBaud;
  lastGPSData = millis();
  return detectedBaud;
}

// Probe a candidate baud: open the UART, feed received bytes to TinyGPS for a
// short window, and report whether any checksum-passing NMEA sentence arrives.
// u-blox emits NMEA continuously even without a fix, so this works before lock.
// Called only from initGPS() during setup(), so blocking delays here are safe
// (no FreeRTOS tasks are running yet).
static bool gpsProbeBaud(unsigned long baud)
{
  ss.end();
  delay(20);
  ss.begin(baud, SERIAL_8N1, pinRX_GPS, pinTX_GPS);

  // Discard any partial/garbage bytes already sitting in the FIFO.
  delay(20);
  while (ss.available() > 0)
  {
    ss.read();
  }

  const unsigned long probeWindowMs = 1500;
  const unsigned long okBefore = gps.passedChecksum();
  const unsigned long startMs = millis();
  while (millis() - startMs < probeWindowMs)
  {
    while (ss.available() > 0)
    {
      gps.encode(ss.read());
    }
    if (gps.passedChecksum() > okBefore)
    {
      return true;
    }
    delay(5);
  }
  return false;
}

float getGPSUpdateFrequency()
{
  return gpsUpdateFrequency;
}

static const char *getGPSBaudCommand(unsigned long baud)
{
  if (baud == 9600UL)
  {
    return PUBX_BAUD_9600;
  }
  if (baud == 38400UL)
  {
    return PUBX_BAUD_38400;
  }
  if (baud == 57600UL)
  {
    return PUBX_BAUD_57600;
  }
  if (baud == 115200UL)
  {
    return PUBX_BAUD_115200;
  }

  return nullptr;
}

static void sendPUBXCommand(const char *commandBody)
{
  uint8_t checksum = 0;

  ss.write('$');
  while (*commandBody != '\0')
  {
    checksum ^= static_cast<uint8_t>(*commandBody);
    ss.write(*commandBody++);
  }

  char checksumSuffix[8];
  snprintf(checksumSuffix, sizeof(checksumSuffix), "*%02X\r\n", checksum);
  ss.print(checksumSuffix);
}

static bool applyGPSBaudRate(unsigned long baud, String &responseMsg)
{
  const char *baudCommand = getGPSBaudCommand(baud);
  if (baudCommand == nullptr)
  {
    responseMsg = "Unsupported GPS baud rate.";
    return false;
  }

  DEBUG_GPS("[GPS Baud] Sending PUBX baud change command for %lu baud.", baud);
  sendPUBXCommand(baudCommand);
  // Do NOT call ss.flush() here. On the ESP32 hardware UART, flush() blocks until
  // the TX FIFO drains and was observed to hang indefinitely on UART2, freezing
  // the calling task so serial and WiFi stop updating. The short PUBX command
  // (<30 bytes) is fully sent well within the delay below.
  delay(1000); // Allow GPS extra time to process PUBX and switch baud internally.

  while (ss.available() > 0)
  {
    ss.read();
  }

  ss.end();
  delay(GPS_COMMAND_SETTLE_MS);
  ss.begin(baud, SERIAL_8N1, pinRX_GPS, pinTX_GPS);
  delay(GPS_COMMAND_SETTLE_MS); // Let the UART settle before any write.

  DEBUG_GPS("[GPS Baud] Stage complete. Local serial restarted at %lu baud.", baud);

  charsProcessedPrevious = gps.charsProcessed(); // snapshot — TinyGPS counter never resets
  passedChecksumPrevious = gps.passedChecksum();
  failedChecksumPrevious = gps.failedChecksum();
  gpsUpdateCount = 0;
  gpsFreqWindowStart = 0;
  gpsUpdateFrequency = 0.0f;
  lastGPSData = millis();
  gpsCurrentSerialBaud = baud;

  responseMsg = "GPS serial switched to " + String(baud) + " baud.";
  return true;
}

bool setGPSUpdateRate(uint8_t rateHz, String &responseMsg)
{
  SemaphoreHandle_t serialMutex = getGPSSerialMutex();
  if ((serialMutex == nullptr) || (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(1500)) != pdTRUE))
  {
    responseMsg = "GPS serial busy.";
    return false;
  }

  DEBUG_GPS("[GPS Rate] Requested update rate: %u Hz. Current baud: %lu, target baud: %lu",
            rateHz, gpsCurrentSerialBaud, (rateHz == 1) ? GPS_DEFAULT_BAUD : GPS_HIGH_RATE_BAUD);

  const bool rateChanged = (gpsUpdateRateHz != rateHz);
  const unsigned long targetBaud = (rateHz == 1) ? GPS_DEFAULT_BAUD : GPS_HIGH_RATE_BAUD;
  const uint8_t *cmd = nullptr;
  size_t len = 0;

  if (rateHz == 1)
  {
    cmd = UBX_1HZ;
    len = sizeof(UBX_1HZ);
  }
  else if (rateHz == 5)
  {
    cmd = UBX_5HZ;
    len = sizeof(UBX_5HZ);
  }
  else if (rateHz == 10)
  {
    cmd = UBX_10HZ;
    len = sizeof(UBX_10HZ);
  }
  else if (rateHz == 16)
  {
    cmd = UBX_16HZ;
    len = sizeof(UBX_16HZ);
  }
  else
  {
    responseMsg = "Invalid rate. Choose 1, 5, 10, or 16 Hz.";
    xSemaphoreGive(serialMutex);
    return false;
  }

  String baudResponse;

  if (targetBaud != gpsCurrentSerialBaud)
  {
    // A baud change is required between current and target.
    if (targetBaud > gpsCurrentSerialBaud)
    {
      // Switching to a higher baud (e.g. 9600 → 38400).
      // First settle the GPS at 1 Hz so the baud-switch PUBX command is not
      // drowned out by the high-rate output the GPS would start producing
      // immediately after the rate command. Mirrors the SpeedPulserPro sequence.
      DEBUG_GPS("[GPS Rate] Settling at 1 Hz before baud switch.");
      for (size_t i = 0; i < sizeof(UBX_1HZ); ++i)
      {
        ss.write(UBX_1HZ[i]);
      }
      // No ss.flush(): HardwareSerial flush() can hang on UART2; the delay covers TX.
      delay(GPS_COMMAND_DELAY_MS);
    }

    // Apply the baud switch (sends PUBX to GPS, restarts serial, resets counters).
    if (!applyGPSBaudRate(targetBaud, baudResponse))
    {
      DEBUG_GPS("[GPS Rate] Baud apply failed: %s", baudResponse.c_str());
      responseMsg = baudResponse;
      xSemaphoreGive(serialMutex);
      return false;
    }

    // Serial is now at targetBaud. Send the target rate command at the new baud.
    DEBUG_GPS("[GPS Rate] Sending UBX rate cmd at new baud (%u bytes).", len);
    for (size_t i = 0; i < len; ++i)
    {
      ss.write(cmd[i]);
    }
    // No ss.flush(): HardwareSerial flush() can hang on UART2; the delay covers TX.
    delay(GPS_COMMAND_DELAY_MS);
  }
  else
  {
    // No baud change needed. Send the rate command at the current baud,
    // then refresh the serial state (clears buffers, resets counters).
    DEBUG_GPS("[GPS Rate] Sending UBX rate cmd at current baud (%u bytes).", len);
    for (size_t i = 0; i < len; ++i)
    {
      ss.write(cmd[i]);
    }
    // No ss.flush(): HardwareSerial flush() can hang on UART2; the delay covers TX.
    delay(GPS_COMMAND_DELAY_MS);
    // Same baud: applyGPSBaudRate sends a no-op PUBX, restarts SS, resets counters.
    applyGPSBaudRate(targetBaud, baudResponse);
  }

  gpsUpdateRateHz = rateHz;
  if (rateChanged)
  {
    pref.putUChar("gpsUpdateRateHz", rateHz);
  }

  xSemaphoreGive(serialMutex);



  responseMsg = "GPS update rate set to " + String(rateHz) + "Hz and " + baudResponse;
  DEBUG_GPS("[GPS Rate] Stage complete: %s", responseMsg.c_str());
  return true;
}

// Returns seconds remaining until the auto rate apply will fire.
//  -1 = no pending auto-apply (rate already applied, or stored rate is 1 Hz)
//   0 = timer hasn't started yet (no satellites seen)
//  >0 = seconds left until it fires
int gpsAutoApplySecondsRemaining()
{
  if (gpsRateApplied || gpsUpdateRateHz <= 1)
  {
    return -1;
  }
  if (gpsSatStableStartMs == 0)
  {
    return (int)(GPS_SAT_STABLE_MS / 1000UL);
  }
  unsigned long elapsed = millis() - gpsSatStableStartMs;
  if (elapsed >= GPS_SAT_STABLE_MS)
  {
    return 0;
  }
  return (int)((GPS_SAT_STABLE_MS - elapsed + 999UL) / 1000UL);
}

// One-shot flag: returns true (and clears the flag) when the GPS task has
// queued an auto rate apply that must be handled from loop().
bool gpsAutoRateApplyPending()
{
  if (gpsPendingRateApply)
  {
    gpsPendingRateApply = false;
    return true;
  }
  return false;
}

void parseGPS(void *args)
{
  while (1)
  {
#if detailedDebugStack
    stackparseGPS = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

    SemaphoreHandle_t serialMutex = getGPSSerialMutex();
    if ((serialMutex != nullptr) && (xSemaphoreTake(serialMutex, 0) == pdTRUE))
    {
      while (ss.available() > 0)
      {
        gps.encode(ss.read());
      }
      xSemaphoreGive(serialMutex);
    }

    unsigned long charsProcessedCurrent = gps.charsProcessed();
    bool gotNewData = false;
    if (charsProcessedCurrent > charsProcessedPrevious)
    {
      lastGPSData = millis();
      charsProcessedPrevious = charsProcessedCurrent;
      gotNewData = true;
    }

    unsigned long now = millis();
    if (gpsFreqWindowStart == 0)
    {
      gpsFreqWindowStart = now;
      passedChecksumPrevious = gps.passedChecksum();
      failedChecksumPrevious = gps.failedChecksum();
    }

    unsigned long passedChecksumCurrent = gps.passedChecksum();
    unsigned long failedChecksumCurrent = gps.failedChecksum();
    unsigned long parsedSentencesCurrent = passedChecksumCurrent + failedChecksumCurrent;
    unsigned long parsedSentencesPrevious = passedChecksumPrevious + failedChecksumPrevious;
    if (parsedSentencesCurrent > parsedSentencesPrevious)
    {
      gpsUpdateCount += (parsedSentencesCurrent - parsedSentencesPrevious);
      passedChecksumPrevious = passedChecksumCurrent;
      failedChecksumPrevious = failedChecksumCurrent;
    }

    if (now - gpsFreqWindowStart >= 1000)
    {
      if (gpsUpdateCount > 0)
      {
        // Divide sentences/sec by 10 (u-blox sends ~9 NMEA sentences per fix)
        // and ceiling so 1Hz→1, 5Hz→5. Only update when sentences received so
        // the value doesn't flicker to 0 between bursts.
        gpsUpdateFrequency = int(gpsUpdateCount * 100.0f / (now - gpsFreqWindowStart) + 1);
      }
      gpsUpdateCount = 0;
      gpsFreqWindowStart = now;
    }

    if (useGPS && ((millis() - lastGPSData) > 10000))
    {
      gpsUnavailable = true;
      gpsError = true;
      hasGPS = false;
      gpsSatellites = 0;
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (gotNewData && gps.satellites.value() == 0)
    {
      gpsUnavailable = false;
      gpsError = false;
      hasGPS = false;
      gpsSatellites = 0;
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (gps.satellites.value() > 0)
    {
      if (!gpsRateApplied && gpsUpdateRateHz > 1)
      {
        if (gpsSatStableStartMs == 0)
        {
          gpsSatStableStartMs = millis();
          DEBUG_GPS("[GPS Auto] Satellites seen - %lus stability timer started.", GPS_SAT_STABLE_MS / 1000UL);
        }
        // Once stable for GPS_SAT_STABLE_MS, flag a rate apply (handled outside parseGPS
        // in loop() with tasks suspended, so delay() calls in setGPSUpdateRate are safe).
        if (millis() - gpsSatStableStartMs >= GPS_SAT_STABLE_MS)
        {
          DEBUG_GPS("[GPS Auto] %lus satellite lock - queuing rate apply at %u Hz.", GPS_SAT_STABLE_MS / 1000UL, gpsUpdateRateHz);
          gpsRateApplied = true;
          gpsPendingRateApply = true;
        }
      }
      else if (!gpsRateApplied)
      {
        gpsRateApplied = true; // Rate is 1 Hz — no apply needed.
      }

      gpsUnavailable = false;
      gpsError = false;
      hasGPS = true;
      gpsSatellites = gps.satellites.value();
      gpsSpeed = int(gps.speed.kmph());
    }
    else
    {
      hasGPS = false;
      gpsSatellites = 0;
      gpsSatStableStartMs = 0; // Reset timer if satellites lost before stable.
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3
                         : vi >= 10    ? 2
                                       : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
}