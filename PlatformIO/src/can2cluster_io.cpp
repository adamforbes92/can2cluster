#include "can2cluster_io.h"
#include <driver/ledc.h>

namespace
{
constexpr ledc_mode_t LEDC_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_bit_t LEDC_RESOLUTION = LEDC_TIMER_10_BIT;

constexpr ledc_timer_t LEDC_SPEED_TIMER = LEDC_TIMER_0;
constexpr ledc_channel_t LEDC_SPEED_CHANNEL = LEDC_CHANNEL_0;

constexpr ledc_timer_t LEDC_RPM_TIMER = LEDC_TIMER_1;
constexpr ledc_channel_t LEDC_RPM_COIL_CHANNEL = LEDC_CHANNEL_1;
constexpr ledc_channel_t LEDC_RPM_PIN_CHANNEL = LEDC_CHANNEL_2;

constexpr uint32_t LEDC_DUTY_OFF = 0;
constexpr uint32_t LEDC_DUTY_50 = 512; // 50% duty with 10-bit resolution (1024 levels)
constexpr uint32_t LEDC_MIN_FREQ_HZ = 2;

void setupLedcOutputs()
{
  ledc_timer_config_t speedTimerConfig = {};
  speedTimerConfig.speed_mode = LEDC_MODE;
  speedTimerConfig.timer_num = LEDC_SPEED_TIMER;
  speedTimerConfig.duty_resolution = LEDC_RESOLUTION;
  speedTimerConfig.freq_hz = LEDC_MIN_FREQ_HZ;
  speedTimerConfig.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&speedTimerConfig);

  ledc_channel_config_t speedChannelConfig = {};
  speedChannelConfig.gpio_num = pinSpeed;
  speedChannelConfig.speed_mode = LEDC_MODE;
  speedChannelConfig.channel = LEDC_SPEED_CHANNEL;
  speedChannelConfig.intr_type = LEDC_INTR_DISABLE;
  speedChannelConfig.timer_sel = LEDC_SPEED_TIMER;
  speedChannelConfig.duty = LEDC_DUTY_OFF;
  speedChannelConfig.hpoint = 0;
  ledc_channel_config(&speedChannelConfig);

  ledc_timer_config_t rpmTimerConfig = {};
  rpmTimerConfig.speed_mode = LEDC_MODE;
  rpmTimerConfig.timer_num = LEDC_RPM_TIMER;
  rpmTimerConfig.duty_resolution = LEDC_RESOLUTION;
  rpmTimerConfig.freq_hz = LEDC_MIN_FREQ_HZ;
  rpmTimerConfig.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&rpmTimerConfig);

  ledc_channel_config_t coilChannelConfig = {};
  coilChannelConfig.gpio_num = pinCoil;
  coilChannelConfig.speed_mode = LEDC_MODE;
  coilChannelConfig.channel = LEDC_RPM_COIL_CHANNEL;
  coilChannelConfig.intr_type = LEDC_INTR_DISABLE;
  coilChannelConfig.timer_sel = LEDC_RPM_TIMER;
  coilChannelConfig.duty = LEDC_DUTY_OFF;
  coilChannelConfig.hpoint = 0;
  ledc_channel_config(&coilChannelConfig);

  ledc_channel_config_t rpmPinChannelConfig = {};
  rpmPinChannelConfig.gpio_num = pinRPM;
  rpmPinChannelConfig.speed_mode = LEDC_MODE;
  rpmPinChannelConfig.channel = LEDC_RPM_PIN_CHANNEL;
  rpmPinChannelConfig.intr_type = LEDC_INTR_DISABLE;
  rpmPinChannelConfig.timer_sel = LEDC_RPM_TIMER;
  rpmPinChannelConfig.duty = LEDC_DUTY_OFF;
  rpmPinChannelConfig.hpoint = 0;
  ledc_channel_config(&rpmPinChannelConfig);
}
}

void basicInit()
{
// basic initialisation - setup pins for IO & setup CAN for receiving...

// if ANY Serial request is made, begin Serial
#if serialDebug || serialDebugWifi || serialDebugEEP || serialDebugGPS || ChassisCANDebug || serialDebugPaddles || serialDebugIO
  Serial.begin(baudSerial);
  delay(500);
  DEBUG("CAN-BUS to Cluster Initialising...");
#endif

#if serialDebug
  DEBUG("Reading EEPROM...");
#endif
  readEEP(); // read EEPROM
#if serialDebug
  DEBUG("Read EEPROM!");
#endif

  ss.begin(baudGPS); // begin GPS Module
#if serialDebugGPS
  // DEBUG(TinyGPSPlus::libraryVersion());
  DEBUG("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum");
  DEBUG("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  DEBUG("----------------------------------------------------------------------------------------------------------------------------------------");
#endif

#if serialDebug
  DEBUG("Setting up IO (pins & buttons)...");
#endif
  setupButtons(); // setup buttons for interrupt first (installs GPIO ISR service)
  setupPins();    // begin IO
#if serialDebug
  DEBUG("Setup IO Complete!");
#endif

#if serialDebug
  DEBUG("CAN Chip Initialising...");
#endif
  canInit(); // initialise the CAN chip
#if serialDebug
  DEBUG("CAN Chip Initialised!");
#endif
}

void setupPins()
{
  // define pin modes for outputs
  pinMode(onboardLED, OUTPUT); // use the built-in LED for displaying errors!

  pinMode(pinSpeed, OUTPUT);   // for speed output
  pinMode(pinEML, OUTPUT);     // for engine management light output
  pinMode(pinEPC, OUTPUT);     // for electronic pedal control output
  pinMode(pinReverse, OUTPUT); // for reverse MOSFET output (5A max!)

  pinMode(pinCoil, OUTPUT); // for high-voltage RPM (can be turned on/off in WiFi so always enable regardless)
  pinMode(pinRPM, OUTPUT);  // for standard square wave RPM
  pinMode(pinRpmPulse, INPUT);

  // Hold generated outputs low until an active frequency is requested.
  digitalWrite(pinCoil, LOW);
  digitalWrite(pinRPM, LOW);
  digitalWrite(pinSpeed, LOW);

  setupLedcOutputs();

  attachInterrupt(digitalPinToInterrupt(pinHallSensor), incomingHz, FALLING); // setup interrupt to toggle pin on change
  attachInterrupt(digitalPinToInterrupt(pinRpmPulse), incomingRPMHz, FALLING); // setup interrupt for engine RPM pulse input
}

void setupButtons()
{
  // Bind single-click events for paddle buttons.
  btnPadUp.attachClick(padUpFunc);
  btnPadDown.attachClick(padDownFunc);
}

void needleSweep()
{
  // Suspend the output tasks so they don't overwrite frequencies mid-sweep
  if (updateSpeedHandle != NULL) vTaskSuspend(updateSpeedHandle);
  if (updateRPMHandle   != NULL) vTaskSuspend(updateRPMHandle);

  const uint16_t effectiveSweepSpeed = sweepSpeed > 0 ? sweepSpeed : 1;
  const uint16_t effectiveStepSpeed = stepSpeed > 0 ? stepSpeed : 1;
  const uint16_t effectiveStepRPM = stepRPM > 0 ? stepRPM : 1;

  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(0);
  setFrequencySpeed(0);

  // Ramp UP in fixed increments with fixed delay between steps.
  long currentSpeed = 0;
  long currentRPM = 0;
  while (currentSpeed < (long)maxSpeed || currentRPM < (long)maxRPM)
  {
    if (currentSpeed < (long)maxSpeed)
    {
      currentSpeed += effectiveStepSpeed;
      if (currentSpeed > (long)maxSpeed)
      {
        currentSpeed = (long)maxSpeed;
      }
    }

    if (currentRPM < (long)maxRPM)
    {
      currentRPM += effectiveStepRPM;
      if (currentRPM > (long)maxRPM)
      {
        currentRPM = (long)maxRPM;
      }
    }

    frequencySpeed = currentSpeed;
    frequencyRPM = currentRPM;
    setFrequencySpeed(currentSpeed);
    setFrequencyRPM(currentRPM);
    vTaskDelay(pdMS_TO_TICKS(effectiveSweepSpeed));
  }

  vTaskDelay(pdMS_TO_TICKS(effectiveSweepSpeed * 2));

  // Ramp DOWN in fixed decrements with fixed delay between steps.
  while (currentSpeed > 0 || currentRPM > 0)
  {
    if (currentSpeed > 0)
    {
      currentSpeed -= effectiveStepSpeed;
      if (currentSpeed < 0)
      {
        currentSpeed = 0;
      }
    }

    if (currentRPM > 0)
    {
      currentRPM -= effectiveStepRPM;
      if (currentRPM < 0)
      {
        currentRPM = 0;
      }
    }

    frequencySpeed = currentSpeed;
    frequencyRPM = currentRPM;
    setFrequencySpeed(currentSpeed);
    setFrequencyRPM(currentRPM);
    vTaskDelay(pdMS_TO_TICKS(effectiveSweepSpeed));
  }

  vTaskDelay(pdMS_TO_TICKS(effectiveSweepSpeed * 2));
  frequencyRPM = 0;
  frequencySpeed = 0;
  setFrequencyRPM(0);
  setFrequencySpeed(0);

  // Resume output tasks now that sweep is complete
  if (updateSpeedHandle != NULL) vTaskResume(updateSpeedHandle);
  if (updateRPMHandle   != NULL) vTaskResume(updateRPMHandle);
}

void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML, bool boolRPM, bool boolSpeed)
{
  // Schedule or retrigger a blink sequence without blocking loop/tasks.
  if (duration <= 0 || flashes <= 0)
  {
    return;
  }

  blinkState.active = true;
  blinkState.flashCount = flashes;
  blinkState.currentFlash = 0;
  blinkState.duration = duration;
  blinkState.lastToggleTime = millis();
  blinkState.outputState = false; // start with outputs OFF
  blinkState.boolEPC = boolEPC;
  blinkState.boolEML = boolEML;
  blinkState.boolRPM = boolRPM;
  blinkState.boolSpeed = boolSpeed;

  // Ensure requested outputs start LOW before first ON edge.
  if (blinkState.boolEPC)
  {
    digitalWrite(pinEPC, LOW);
  }
  if (blinkState.boolEML)
  {
    digitalWrite(pinEML, LOW);
  }
  if (blinkState.boolRPM)
  {
    digitalWrite(pinRPM, LOW);
  }
  if (blinkState.boolSpeed)
  {
    digitalWrite(pinSpeed, LOW);
  }
}

void updateBlinkLED(void)
{
  // Non-blocking blink update function - call this from main loop
  if (!blinkState.active)
  {
    return; // No blink sequence running
  }

  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - blinkState.lastToggleTime;

  if (elapsedTime >= blinkState.duration)
  {
    // Time to toggle
    blinkState.lastToggleTime = currentTime;
    blinkState.outputState = !blinkState.outputState; // toggle state

    // Apply the current state to all requested outputs
    if (blinkState.boolEPC)
    {
      digitalWrite(pinEPC, blinkState.outputState ? HIGH : LOW);
    }
    if (blinkState.boolEML)
    {
      digitalWrite(pinEML, blinkState.outputState ? HIGH : LOW);
    }
    if (blinkState.boolRPM)
    {
      digitalWrite(pinRPM, blinkState.outputState ? HIGH : LOW);
    }
    if (blinkState.boolSpeed)
    {
      digitalWrite(pinSpeed, blinkState.outputState ? HIGH : LOW);
    }

    // Count flashes (each ON->OFF is one flash)
    if (!blinkState.outputState)
    {
      blinkState.currentFlash++;
    }

    // Check if we're done
    if (blinkState.currentFlash >= blinkState.flashCount)
    {
      // Sequence complete - turn off all outputs
      digitalWrite(pinEPC, LOW);
      digitalWrite(pinEML, LOW);
      digitalWrite(pinRPM, LOW);
      digitalWrite(pinSpeed, LOW);
      blinkState.active = false;
    }
  }
}

void diagTest()
{
  vehicleRPM += 1000;
  vehicleSpeed += 10;
  ecuSpeed += 10;
  absSpeed += 12;
  dsgSpeed += 14;
  gpsSpeed += 16;
  dsgUDSSpeed += 18;
  hallSpeed += 20;

  if (vehicleRPM > clusterRPMLimit)
  {
    vehicleRPM = 1000;
    frequencyRPM = 1;
  }
  if (vehicleSpeed > maxSpeed)
  {
    vehicleSpeed = 1;
    frequencySpeed = 1;
  }
  if (hallSpeed > maxSpeed)
  {
    hallSpeed = 1;
    frequencySpeed = 1;
  }
  if (ecuSpeed > maxSpeed)
  {
    ecuSpeed = 1;
    frequencySpeed = 1;
  }
  if (absSpeed > maxSpeed)
  {
    absSpeed = 1;
    frequencySpeed = 1;
  }
  if (dsgSpeed > maxSpeed)
  {
    dsgSpeed = 1;
    frequencySpeed = 1;
  }
  if (dsgUDSSpeed > maxSpeed)
  {
    dsgUDSSpeed = 1;
    frequencySpeed = 1;
  }
  if (gpsSpeed > maxSpeed)
  {
    gpsSpeed = 1;
    frequencySpeed = 1;
  }

  vehicleReverse = !vehicleReverse;
  hasCAN = !hasCAN;
  hasGPS = !hasGPS;
  vehicleEML = !vehicleEML;
  vehicleEPC = !vehicleEPC;
  vehiclePark = !vehiclePark;

  blinkLED(500, 1, 1, 1, 0, 0);
}

// adjust output frequency
void setFrequencyRPM(long frequencyHz)
{
  ledc_channel_t activeChannel = coilType ? LEDC_RPM_COIL_CHANNEL : LEDC_RPM_PIN_CHANNEL;
  ledc_channel_t inactiveChannel = coilType ? LEDC_RPM_PIN_CHANNEL : LEDC_RPM_COIL_CHANNEL;

  ledc_set_duty(LEDC_MODE, inactiveChannel, LEDC_DUTY_OFF);
  ledc_update_duty(LEDC_MODE, inactiveChannel);

  if (frequencyHz > 0)
  {
    uint32_t targetFreq = static_cast<uint32_t>(frequencyHz);
    if (targetFreq < LEDC_MIN_FREQ_HZ)
    {
      targetFreq = LEDC_MIN_FREQ_HZ;
    }
    ledc_set_freq(LEDC_MODE, LEDC_RPM_TIMER, targetFreq);
    ledc_set_duty(LEDC_MODE, activeChannel, LEDC_DUTY_50);
    ledc_update_duty(LEDC_MODE, activeChannel);
  }
  else
  {
    ledc_set_duty(LEDC_MODE, activeChannel, LEDC_DUTY_OFF);
    ledc_update_duty(LEDC_MODE, activeChannel);
  }
}

// adjust output frequency
void setFrequencySpeed(long frequencyHz)
{
  if (frequencyHz > 0)
  {
    uint32_t targetFreq = static_cast<uint32_t>(frequencyHz);
    if (targetFreq < LEDC_MIN_FREQ_HZ)
    {
      targetFreq = LEDC_MIN_FREQ_HZ;
    }
    ledc_set_freq(LEDC_MODE, LEDC_SPEED_TIMER, targetFreq);
    ledc_set_duty(LEDC_MODE, LEDC_SPEED_CHANNEL, LEDC_DUTY_50);
    ledc_update_duty(LEDC_MODE, LEDC_SPEED_CHANNEL);
  }
  else
  {
    ledc_set_duty(LEDC_MODE, LEDC_SPEED_CHANNEL, LEDC_DUTY_OFF);
    ledc_update_duty(LEDC_MODE, LEDC_SPEED_CHANNEL);
  }
}

void incomingHz()
{                                                                // Interrupt 0 service routine
  static unsigned long previousMicros = micros();                // remember variable, initialize first time
  unsigned long presentMicros = micros();                        // read microseconds
  unsigned long revolutionTime = presentMicros - previousMicros; // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL)
    return;                                               // avoid divide by 0, also debounce, speed can't be over 60,000 was 1000UL
  dutyCycleIncoming = (60000000UL / revolutionTime) / 60; // calculate
  previousMicros = presentMicros;
  lastPulse = millis();

  hallSpeed = map(dutyCycleIncoming, 0, maxFreqHall, 0, maxSpeed); // map incoming range to this codes range.  Max Hz should match Max Speed - i.e., 200Hz = 200kmh, or 500Hz = 200kmh...
}

void incomingRPMHz()
{                                                                // Interrupt service routine for incoming RPM pulse
  static unsigned long previousMicros = micros();                // remember variable, initialize first time
  unsigned long presentMicros = micros();                        // read microseconds
  unsigned long revolutionTime = presentMicros - previousMicros; // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL)
    return;                                                    // avoid divide by 0, also debounce
  dutyCycleMotor = (60000000UL / revolutionTime) / 60;        // calculate incoming frequency in Hz
  previousMicros = presentMicros;
}