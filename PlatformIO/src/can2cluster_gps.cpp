#include "can2cluster_gps.h"

void parseGPS(void *args) {
  // Keep parseGPS task active and set its own sleep state when no GPS is found.
  while (1) {
#if detailedDebugStack
    stackparseGPS = uxTaskGetStackHighWaterMark(NULL);  // for capturing how much memory the task is using
#endif

    // Read GPS bytes if available
    bool readAny = false;
    while (ss.available() > 0) {
      gps.encode(ss.read());
      readAny = true;
      lastGPSCharMillis = millis();
    }

    if (readAny) {
      gpsError = false;
    }

    // If no data for a threshold, mark missing and suspend this task.
    if ((millis() - lastGPSCharMillis) > 10000) {
      if (!gpsTaskSuspended) {
        gpsError = true;
        hasGPS = false;
        gpsSatellites = 0;
        gpsTaskSuspended = true;
        DEBUG_GPS("No data for 10s, suspending parseGPS task");
        vTaskSuspend(NULL);
      }
    } else {
      if (gps.satellites.value() > 0) {
        hasGPS = true;
        gpsSatellites = gps.satellites.value();
      } else {
        hasGPS = false;
        gpsSatellites = 0;
      }

      if (gps.speed.isUpdated()) {
        gpsSpeed = int(gps.speed.kmph());
      }
    }

    vTaskDelay(pdMS_TO_TICKS(200));  // reduce polling overhead
  }
}

void gpsResumeTask(void *args) {
  while (1) {
    if (gpsTaskSuspended) {
      if (ss.available() > 0) {
        lastGPSCharMillis = millis();
        gpsTaskSuspended = false;
        gpsError = false;
        hasGPS = true;
        if (gpsTaskHandle != NULL) {
          vTaskResume(gpsTaskHandle);
          DEBUG_GPS("Data received, resuming parseGPS task");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

static void printFloat(float val, bool valid, int len, int prec) {
  if (!valid) {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  } else {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1);  // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3
                           : vi >= 10  ? 2
                                       : 1;
    for (int i = flen; i < len; ++i)
      Serial.print(' ');
  }
}