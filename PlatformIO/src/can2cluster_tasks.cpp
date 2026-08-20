#include "can2cluster_io.h"
#include "can2cluster_uds.h"
#include "can2cluster_i2c.h"

void setupTasks()
{
  // Core 0: Serial output only — keep free for the WiFi stack
  xTaskCreatePinnedToCore(showState,    "showState",  8000, NULL,  1, NULL,          0);

  // Core 1: All IO — CAN TX, GPS, speed/RPM output, EEPROM, DSG calc
  xTaskCreatePinnedToCore(writeEEP,       "writeEEP",       2000, NULL,  2, NULL,    1);
  xTaskCreatePinnedToCore(taskTP20,       "taskTP20",       4096, NULL,  3, NULL,    1);
  xTaskCreatePinnedToCore(taskUDS,        "taskUDS",        4096, NULL,  3, NULL,    1);
  xTaskCreatePinnedToCore(parseDSG,       "parseDSG",       6000, NULL,  4, NULL,    1);
  xTaskCreatePinnedToCore(broadcastGRA,   "broadcastGRA",   4000, NULL,  5, NULL,    1);
  xTaskCreatePinnedToCore(broadcastSpeed, "broadcastSpeed", 4000, NULL,  6, NULL,    1);
  xTaskCreatePinnedToCore(parseGPS,       "parseGPS",       6000, NULL,  7, &gpsTaskHandle, 1);
  xTaskCreatePinnedToCore(updateSpeed,    "updateSpeed",    4096, NULL,  9, &updateSpeedHandle, 1);
  xTaskCreatePinnedToCore(updateRPM,      "updateRPM",      4096, NULL, 10, &updateRPMHandle,   1);
  xTaskCreatePinnedToCore(outputControlTask, "outputControlTask", 4096, NULL, 11, NULL, 1);
  xTaskCreatePinnedToCore(paddleFeedbackTask, "paddleFeedbackTask", 2048, NULL, 11, NULL, 1);
  xTaskCreatePinnedToCore(checkError,     "checkError",     2000, NULL, 12, NULL,    1);
  xTaskCreatePinnedToCore(diagTestTask,   "diagTestTask",   3000, NULL,  8, NULL,    1);
}

// Suspend all tasks that could contend for the GPS UART (or be starved by the
// blocking delays in setGPSUpdateRate) during a GPS baud/rate change.
// Called from loop() before setGPSUpdateRate().
void tasksSuspendAll()
{
  if (gpsTaskHandle != NULL)
    vTaskSuspend(gpsTaskHandle);
  if (updateSpeedHandle != NULL)
    vTaskSuspend(updateSpeedHandle);
  if (updateRPMHandle != NULL)
    vTaskSuspend(updateRPMHandle);
  DEBUG("[TASK] Tasks suspended for GPS rate change.");
}

// Resume all tasks suspended by tasksSuspendAll().
void tasksResumeAll()
{
  if (gpsTaskHandle != NULL)
    vTaskResume(gpsTaskHandle);
  if (updateSpeedHandle != NULL)
    vTaskResume(updateSpeedHandle);
  if (updateRPMHandle != NULL)
    vTaskResume(updateRPMHandle);
  DEBUG("[TASK] Tasks resumed after GPS rate change.");
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
    DEBUG_STACK("Stack Sizes:");
    DEBUG_STACK("    stackShowState: %d", stackShowState);       // incrememting value for checking the response to vars...
    DEBUG_STACK("    stackUpdateLabels: %d", stackUpdateLabels); // incrememting value for checking the response to vars...

    DEBUG_STACK("    stackWriteEEP: %d", stackWriteEEP); // incrememting value for checking the response to vars...

    DEBUG_STACK("    stackbroadcastGRA: %d", stackbroadcastGRA);     // incrememting value for checking the response to vars...
    DEBUG_STACK("    stackbroadcastSpeed: %d", stackbroadcastSpeed); // incrememting value for checking the response to vars...
    DEBUG_STACK("    stackparseGPS: %d", stackparseGPS);             // incrememting value for checking the response to vars...
    DEBUG_STACK("    stackparseDSG: %d", stackparseDSG);             // incrememting value for checking the response to vars...

    DEBUG_STACK("    stackupdateSpeed: %d", stackupdateSpeed); // incrememting value for checking the response to vars...
    DEBUG_STACK("    stackupdateRPM: %d", stackupdateRPM);     // incrememting value for checking the response to vars...
    DEBUG_STACK("    stackshiftLight: %d", stackshiftLight);   // incrememting value for checking the response to vars...

    DEBUG_STACK("    stackcheckError: %d", stackcheckError); // incrememting value for checking the response to vars...
#endif

#if ChassisCANDebug
    DEBUG_CHASSIS_CAN("From CAN:");
    DEBUG_CHASSIS_CAN("  vehicleRPM: %d", vehicleRPM);
    DEBUG_CHASSIS_CAN("  vehicleSpeed: %d", vehicleSpeed);
    DEBUG_CHASSIS_CAN("  vehicleReverse: %d", vehicleReverse);
    DEBUG_CHASSIS_CAN("  vehicleEML: %d", vehicleEML);
    DEBUG_CHASSIS_CAN("  vehicleEPC: %d", vehicleEPC);
#endif

#if serialDebugGPS
    DEBUG_GPS("From GPS:");
    DEBUG_GPS("  Satellites: %d", gps.satellites.value());
    DEBUG_GPS("  gpsSpeed: %d", gpsSpeed);
#endif

#if serialDebugIO
    DEBUG_IO("Speeds:");
    DEBUG_IO("  hallSpeed: %d", dutyCycleIncoming);
    DEBUG_IO("  vrSpeed: %d", dutyCycleIncomingVR);
    DEBUG_IO("  ecuSpeed: %d", calcSpeed);
    DEBUG_IO("  dsgSpeed: %d", dsgSpeed);
    DEBUG_IO("  gpsSpeed: %d", gpsSpeed);
    DEBUG_IO("  absSpeed: %d", absSpeed);
#endif

    i2cSerialDiag();

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

    if (!tempNeedleSweep && !diagTest)
    { // only here if tested in WiFi

      // Reset Hall speed if no pulse has arrived within durationReset ms
      if ((millis() + 10 - lastPulse) > durationReset)
      {
        dutyCycleIncoming = 0;
        hallSpeed = 0;
      }
      else
      {
        hallSpeed = map(dutyCycleIncoming, 0, maxFreqHall, 0, maxSpeed);
      }

      // Reset VR speed if no pulse has arrived within durationReset ms
      if ((millis() + 10 - lastPulseVR) > durationReset)
      {
        dutyCycleIncomingVR = 0;
        vrSpeed = 0;
      }
      else
      {
        vrSpeed = map(dutyCycleIncomingVR, 0, maxFreqVR, 0, maxSpeed);
      }

      if (testSpeedo)
      {
        vehicleSpeed = tempSpeed;
      }
      else
      {
        vehicleSpeed = 0; // reset so that if no source is active, output goes to zero
        if (useHall)
        {
          vehicleSpeed = hallSpeed;
        }
        if (useVR)
        {
          vehicleSpeed = vrSpeed;
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
        if (useTP20)
        {
          vehicleSpeed = int(tp20Speed);
        }
        if (useUDS)
        {
          vehicleSpeed = int(udsSpeed);
        }
        if (useGPS)
        {
          vehicleSpeed = int(gpsSpeed);
        }
        if (useAftermarket)
        {
          vehicleSpeed = int(aftermarketSpeed);
        }
      }

      if (useMPH)
      {
        vehicleSpeed = int(((uint32_t)vehicleSpeed * mphFactor) / 1000000); // km/h -> MPH (factor 0.621371)
      }

      // calculate final frequency:
      frequencySpeed = map(vehicleSpeed, 0, maxSpeed, 0, maxSpeed);
      setFrequencySpeed(frequencySpeed); // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'  }
    }
    vTaskDelay(pdMS_TO_TICKS(rpmPause));
  }
}

void updateRPM(void *args)
{
  while (1)
  {
#if detailedDebugStack
    stackupdateRPM = uxTaskGetStackHighWaterMark(NULL); // for capturing how much memory the task is using
#endif
    if (!tempNeedleSweep && !diagTest)
    { // only here if tested in WiFi
      if (testRPM)
      { // set vehicleRPM is testing or not
        vehicleRPM = tempRPM;
      }
      else
      {
      if (useHallRPM)
        {
          // Reset RPM if no pulse has arrived within durationReset ms
          if ((millis() + 10 - lastPulseRPM) > durationReset)
          {
            dutyCycleMotor = 0;
            vehicleRPM = 0;
          }
          else
          {
            unsigned long clampedMotorHz = dutyCycleMotor > maxRPM ? maxRPM : dutyCycleMotor;
            vehicleRPM = map(clampedMotorHz, 0, maxRPM, 0, clusterRPMLimit);
          }
        }
        else
        {
          vehicleRPM = vehicleRPMCAN;
        }
      }

      frequencyRPM = map(vehicleRPM, 0, clusterRPMLimit, 0, maxRPM);
      setFrequencyRPM(frequencyRPM); // minimum speed may command 0 and setFreq. will cause crash, so +1 to error 'catch'
    }
    vTaskDelay(pdMS_TO_TICKS(rpmPause));
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
      DEBUG_IO("tempShiftLight");
#endif
    }

    // Trigger shift flashes while RPM remains above configured threshold.
    if (!diagTest && (useEPCShiftLight || useEMLShiftLight) && vehicleRPM > shiftLimit)
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

    // The coolant gauge (if enabled) owns one of the EML/EPC pins as a PWM
    // output on the OLD board — skip the digital drive for that pin and let
    // updateCoolantOutput handle it. On the new board coolant is a separate DAC.
    bool coolantOwnsEML = (!isNewBoard && coolantOutput == 1);
    bool coolantOwnsEPC = (!isNewBoard && coolantOutput == 2);

    // Normal EML/EPC drive, unless the pin is owned by the blink sequence, the
    // coolant PWM gauge, or held by a diagnostic test (handled below).
    if (!blinkOwnsEML && !coolantOwnsEML && !testEML)
    {
      driveEML(finalEML);
    }
    if (!blinkOwnsEPC && !coolantOwnsEPC && !testEPC)
    {
      driveEPC(finalEPC);
    }

    updateCoolantOutput();

    // Diagnostic test outputs are a hard override ("direct short") — driven
    // last so they beat the coolant PWM gauge, shift-light blink and DSG park.
    // updateCoolantOutput() releases the LEDC channel on a tested pin so this
    // plain drive actually reaches the output.
    if (testEML)
      driveEML(true);
    if (testEPC)
      driveEPC(true);
    driveReverse(testReverse ? true : vehicleReverse);

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
