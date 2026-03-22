#include "can2cluster_io.h"

void setupTasks()
{
  // Core 0: Serial output only — keep free for the WiFi stack
  xTaskCreatePinnedToCore(showState,    "showState",  8000, NULL,  1, NULL,          0);

  // Core 1: All IO — CAN TX, GPS, speed/RPM output, EEPROM, DSG calc
  xTaskCreatePinnedToCore(writeEEP,       "writeEEP",       2000, NULL,  2, NULL,    1);
  xTaskCreatePinnedToCore(queryECUTask,   "queryECUTask",   4000, NULL,  3, NULL,    1);
  xTaskCreatePinnedToCore(parseDSG,       "parseDSG",       6000, NULL,  4, NULL,    1);
  xTaskCreatePinnedToCore(broadcastGRA,   "broadcastGRA",   4000, NULL,  5, NULL,    1);
  xTaskCreatePinnedToCore(broadcastSpeed, "broadcastSpeed", 4000, NULL,  6, NULL,    1);
  xTaskCreatePinnedToCore(parseGPS,       "parseGPS",       6000, NULL,  7, &gpsTaskHandle, 1);
  xTaskCreatePinnedToCore(gpsResumeTask,  "gpsResumeTask",  3000, NULL,  8, NULL,    1);
  xTaskCreatePinnedToCore(updateSpeed,    "updateSpeed",    4096, NULL,  9, &updateSpeedHandle, 1);
  xTaskCreatePinnedToCore(updateRPM,      "updateRPM",      4096, NULL, 10, &updateRPMHandle,   1);
  xTaskCreatePinnedToCore(outputControlTask, "outputControlTask", 4096, NULL, 11, NULL, 1);
  xTaskCreatePinnedToCore(paddleFeedbackTask, "paddleFeedbackTask", 2048, NULL, 11, NULL, 1);
  xTaskCreatePinnedToCore(checkError,     "checkError",     2000, NULL, 12, NULL,    1);
}

void showState(void *arg)
{
  while (1)
  {
// stackshowHaldexState = uxTaskGetStackHighWaterMark(NULL);
#if detailedDebugStack
    stackShowState = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

#if detailedDebugStack
    DEBUG("Stack Sizes:");
    DEBUG("    stackShowState: %d", stackShowState);       // incrememting value for checking the response to vars...
    DEBUG("    stackUpdateLabels: %d", stackUpdateLabels); // incrememting value for checking the response to vars...

    DEBUG("    stackWriteEEP: %d", stackWriteEEP); // incrememting value for checking the response to vars...

    DEBUG("    stackbroadcastGRA: %d", stackbroadcastGRA);     // incrememting value for checking the response to vars...
    DEBUG("    stackbroadcastSpeed: %d", stackbroadcastSpeed); // incrememting value for checking the response to vars...
    DEBUG("    stackparseGPS: %d", stackparseGPS);             // incrememting value for checking the response to vars...
    DEBUG("    stackparseDSG: %d", stackparseDSG);             // incrememting value for checking the response to vars...

    DEBUG("    stackupdateSpeed: %d", stackupdateSpeed); // incrememting value for checking the response to vars...
    DEBUG("    stackupdateRPM: %d", stackupdateRPM);     // incrememting value for checking the response to vars...
    DEBUG("    stackshiftLight: %d", stackshiftLight);   // incrememting value for checking the response to vars...

    DEBUG("    stackcheckError: %d", stackcheckError); // incrememting value for checking the response to vars...
#endif

#if ChassisCANDebug
    DEBUG("From CAN:");
    DEBUG("  vehicleRPM: %d", vehicleRPM);
    DEBUG("  vehicleSpeed: %d", vehicleSpeed);
    DEBUG("  vehicleReverse: %d", vehicleReverse);
    DEBUG("  vehicleEML: %d", vehicleEML);
    DEBUG("  vehicleEPC: %d", vehicleEPC);
#endif

#if serialDebugGPS
    DEBUG("From GPS:");
    DEBUG("  Satellites: %d", gps.satellites.value());
    DEBUG("  gpsSpeed: %d", gpsSpeed);
#endif

#if serialDebugIO
    DEBUG("Speeds:");
    DEBUG("  hallSpeed: %d", dutyCycleIncoming);
    DEBUG("  ecuSpeed: %d", calcSpeed);
    DEBUG("  dsgSpeed: %d", dsgSpeed);
    DEBUG("  gpsSpeed: %d", gpsSpeed);
    DEBUG("  absSpeed: %d", absSpeed);
#endif

    vTaskDelay(pdMS_TO_TICKS(serialMonitorRefresh));
  }
}

void updateSpeed(void *args)
{
  while (1)
  {
#if detailedDebugStack
    stackupdateSpeed = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

    if (!tempNeedleSweep)
    { // only here if tested in WiFi
      if (testSpeedo)
      {
        vehicleSpeed = tempSpeed;
      }
      else
      {
        if (useHall)
        {
          vehicleSpeed = hallSpeed;
        }
        if (useECU)
        {
          vehicleSpeed = (int)ecuSpeed;
        }
        if (useABS)
        {
          vehicleSpeed = int(absSpeed);
        }
        if (useDSG)
        {
          vehicleSpeed = int(dsgSpeed);
        }
        if (useTPUDSDSG)
        {
          vehicleSpeed = int(dsgUDSSpeed);
        }
        if (useGPS)
        {
          vehicleSpeed = int(gpsSpeed);
        }
      }

      if (speedUnits == 1)
      {
        vehicleSpeed = int((vehicleSpeed * mphFactor) / 1000000); // 621371
      }

      // calculate final frequency:
      frequencySpeed = map(vehicleSpeed, 0, maxSpeed, 0, maxSpeed);
      setFrequencySpeed(frequencySpeed); // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'  }
    }
    vTaskDelay(1);
  }
}

void updateRPM(void *args)
{
  while (1)
  {
#if detailedDebugStack
    stackupdateRPM = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif
    if (!tempNeedleSweep)
    { // only here if tested in WiFi
      if (testRPM)
      { // set vehicleRPM is testing or not
        vehicleRPM = tempRPM;
      }
      else
      {
        if (useHallRPM)
        {
          unsigned long clampedMotorHz = dutyCycleMotor > maxRPM ? maxRPM : dutyCycleMotor;
          vehicleRPM = map(clampedMotorHz, 0, maxRPM, 0, clusterRPMLimit);
        }
        else
        {
          vehicleRPM = vehicleRPMCAN;
        }
      }

      frequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRPM);
      setFrequencyRPM(frequencyRPM); // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
    }
    vTaskDelay(1);
  }
}

void outputControlTask(void *args)
{
  while (1)
  {
    if (tempShiftLight)
    {
      blinkLED(shiftLightRate, shiftFlashes, useEPCShiftLight, useEMLShiftLight, 0, 0);
      tempShiftLight = false;
#if serialDebugIO
      DEBUG("tempShiftLight");
#endif
    }

    // Trigger shift flashes while RPM remains above configured threshold.
    if ((useEPCShiftLight || useEMLShiftLight) && vehicleRPM > shiftLimit)
    {
      if (!blinkState.active)
      {
        blinkLED(shiftLightRate, shiftFlashes, useEPCShiftLight, useEMLShiftLight, 0, 0);
      }
    }

    bool finalEML = vehicleEML;
    bool finalEPC = vehicleEPC;

    // DSG Park mode overrides normal EML/EPC behavior.
    if (dsgParkMode == "EML" && vehiclePark)
    {
      finalEML = true;
      finalEPC = false;
    }
    else if (dsgParkMode == "EPC" && vehiclePark)
    {
      finalEPC = true;
      finalEML = false;
    }

    updateBlinkLED();

    bool blinkOwnsEML = blinkState.active && blinkState.boolEML;
    bool blinkOwnsEPC = blinkState.active && blinkState.boolEPC;

    if (!blinkOwnsEML)
    {
      testEML ? digitalWrite(pinEML, HIGH) : digitalWrite(pinEML, finalEML);
    }
    if (!blinkOwnsEPC)
    {
      testEPC ? digitalWrite(pinEPC, HIGH) : digitalWrite(pinEPC, finalEPC);
    }
    testReverse ? digitalWrite(pinReverse, HIGH) : digitalWrite(pinReverse, vehicleReverse);

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void paddleFeedbackTask(void *args)
{
  while (1)
  {
    updatePaddleFeedback();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void checkError(void *args)
{
  while (1)
  {
#if detailedDebugStack
    stackcheckError = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif

    // Mark CAN as unhealthy if no frame received in the last 2 seconds
    if ((millis() - lastCAN) > 2000) {
      hasCAN = false;
    }

    if (hasError)
    {
      triggerLED = !triggerLED;
    }
    else
    {
      triggerLED = false;
    }

    if (triggerLED)
    {
      digitalWrite(onboardLED, HIGH); // turn internal LED on
    }
    else
    {
      digitalWrite(onboardLED, LOW); // turn internal LED off
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
