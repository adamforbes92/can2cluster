#include "can2cluster_io.h"
#include "can2cluster_gps.h"
#include <driver/ledc.h>

// File-scope ISR state — must NOT be function-local statics.
// C++ function-local statics with non-constant initialisers call
// __cxa_guard_acquire() -> pthread_mutex_lock() -> xSemaphoreTake()
// on their first invocation, which asserts inside FreeRTOS when triggered
// from an ISR context (xQueueSemaphoreTake assert in queue.c).
static unsigned long hallPreviousMicros = 0;
static unsigned long rpmPreviousMicros  = 0;

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
#if serialDebug || serialDebugWifi || serialDebugEEP || serialDebugGPS || ChassisCANDebug || serialDebugPaddles || serialDebugIO || serialDebugDSG || serialDebugCAN
  Serial.begin(baudSerial);
  delay(500);
#endif
  DEBUG("[Init] CAN-BUS to Cluster Initialising...");

  DEBUG("[Init] Reading EEPROM...");
  readEEP(); // read EEPROM
  DEBUG("[Init] Read EEPROM!");

  initGPS(); // initialise GPS serial at default baud and reset state
#if serialDebugGPS
  // DEBUG_GPS(TinyGPSPlus::libraryVersion());
  DEBUG_GPS("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum");
  DEBUG_GPS("           (deg)      (deg)       Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  DEBUG_GPS("----------------------------------------------------------------------------------------------------------------------------------------");
#endif

  DEBUG("[Init] Setting up IO (pins & buttons)...");
  setupButtons(); // setup buttons for interrupt first (installs GPIO ISR service)
  setupPins();    // begin IO
  DEBUG("[Init] Setup IO Complete!");

  DEBUG("[Init] CAN Chip Initialising...");
  canInit(); // initialise the CAN chip
  DEBUG("[Init] CAN Chip Initialised!");
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

  // Initialise ISR timestamps now so the first pulse doesn't see a
  // stale zero and produce a spurious reading.
  hallPreviousMicros = micros();
  rpmPreviousMicros  = micros();

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
  // Task suspension is handled by the caller (main loop) via tasksSuspendAll() / tasksResumeAll().

  const uint32_t kSweepPollMs = 10;

  // Total fade duration per needle — matches SPP formula:
  //   stepSpeed × sweepSpeed × maxSpeed / 10  (e.g. 10 × 18 × 200 / 10 = 3600 ms)
  const uint32_t kFadeMsSpeedRaw = (uint32_t)(stepSpeed * (float)sweepSpeed * (float)maxSpeed / 10.0f);
  const uint32_t kFadeMsRPMRaw   = (uint32_t)(stepRPM   * (float)sweepSpeed * (float)maxRPM   / 10.0f);
  const uint32_t kFadeMsSpeed    = max<uint32_t>(kFadeMsSpeedRaw, 1U);
  const uint32_t kFadeMsRPM      = max<uint32_t>(kFadeMsRPMRaw,   1U);
  const uint32_t kFadeMsMax      = max(kFadeMsSpeed, kFadeMsRPM);

  const long kMaxSpeedFreq = (long)maxSpeed;
  const long kMaxRpmFreq   = (long)maxRPM;

  // ---- Ramp UP: both needles ramp concurrently within their own time budgets ----
  {
    uint32_t sweepStart = millis();
    while (millis() - sweepStart < kFadeMsMax)
    {
      uint32_t elapsed = millis() - sweepStart;

      if (elapsed < kFadeMsSpeed)
      {
        long targetSpeed = (kMaxSpeedFreq * (long)elapsed) / (long)kFadeMsSpeed;
        frequencySpeed = targetSpeed;
        setFrequencySpeed(targetSpeed);
      }

      if (elapsed < kFadeMsRPM)
      {
        long targetRPM = (kMaxRpmFreq * (long)elapsed) / (long)kFadeMsRPM;
        frequencyRPM = targetRPM;
        setFrequencyRPM(targetRPM);
      }

      vTaskDelay(pdMS_TO_TICKS(kSweepPollMs));
    }
  }

  // Hard-set full deflection
  frequencySpeed = kMaxSpeedFreq;
  frequencyRPM   = kMaxRpmFreq;
  setFrequencySpeed(kMaxSpeedFreq);
  setFrequencyRPM(kMaxRpmFreq);

  // Pause at full deflection
  vTaskDelay(pdMS_TO_TICKS((uint32_t)sweepSpeed * 2));

  // ---- Ramp DOWN: mirror of ramp-up ----------------------------------------
  {
    uint32_t sweepStart = millis();
    while (millis() - sweepStart < kFadeMsMax)
    {
      uint32_t elapsed = millis() - sweepStart;

      if (elapsed < kFadeMsSpeed)
      {
        long targetSpeed = kMaxSpeedFreq - (kMaxSpeedFreq * (long)elapsed) / (long)kFadeMsSpeed;
        if (targetSpeed < 0) targetSpeed = 0;
        frequencySpeed = targetSpeed;
        setFrequencySpeed(targetSpeed);
      }

      if (elapsed < kFadeMsRPM)
      {
        long targetRPM = kMaxRpmFreq - (kMaxRpmFreq * (long)elapsed) / (long)kFadeMsRPM;
        if (targetRPM < 0) targetRPM = 0;
        frequencyRPM = targetRPM;
        setFrequencyRPM(targetRPM);
      }

      vTaskDelay(pdMS_TO_TICKS(kSweepPollMs));
    }
  }

  // Settle, then hard-zero both outputs
  vTaskDelay(pdMS_TO_TICKS((uint32_t)sweepSpeed * 2));
  frequencySpeed = 0;
  frequencyRPM   = 0;
  setFrequencySpeed(0);
  setFrequencyRPM(0);
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

// Bench diagnostic task. While the `diagTest` flag is true, every output is
// driven from this task: vehicleRPM/Speed ramp up, every other speed source
// is bumped (so the WiFi live page shows them), and EML/EPC/Reverse/Park
// toggle so each pin can be verified with a meter. updateSpeed/updateRPM and
// the shift-light blink path check `diagTest` and skip while it's active so
// they don't fight this task.
void diagTestTask(void *args)
{
  while (1)
  {
    if (!diagTest)
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    vehicleRPM   += 500;
    vehicleSpeed += 5;
    ecuSpeed     += 5;
    absSpeed     += 6;
    dsgSpeed     += 7;
    gpsSpeed     += 8;
    dsgUDSSpeed  += 9;
    tp20Speed    += 9;
    udsSpeed     += 9;
    hallSpeed    += 10;

    if (vehicleRPM   > clusterRPMLimit) vehicleRPM   = 1000;
    if (vehicleSpeed > maxSpeed)        vehicleSpeed = 1;
    if (hallSpeed    > maxSpeed)        hallSpeed    = 1;
    if (ecuSpeed     > maxSpeed)        ecuSpeed     = 1;
    if (absSpeed     > maxSpeed)        absSpeed     = 1;
    if (dsgSpeed     > maxSpeed)        dsgSpeed     = 1;
    if (dsgUDSSpeed  > maxSpeed)        dsgUDSSpeed  = 1;
    if (tp20Speed    > maxSpeed)        tp20Speed    = 1;
    if (udsSpeed     > maxSpeed)        udsSpeed     = 1;
    if (gpsSpeed     > maxSpeed)        gpsSpeed     = 1;

    // Drive the cluster outputs directly so updateSpeed/updateRPM staying
    // idle doesn't leave the LEDC frequencies stale.
    frequencyRPM   = map(vehicleRPM,   0, clusterRPMLimit, 0, maxRPM);
    frequencySpeed = map(vehicleSpeed, 0, maxSpeed,        0, maxSpeed);
    setFrequencyRPM(frequencyRPM);
    setFrequencySpeed(frequencySpeed);

    vehicleReverse = !vehicleReverse;
    vehicleEML     = !vehicleEML;
    vehicleEPC     = !vehicleEPC;
    vehiclePark    = !vehiclePark;
    hasCAN         = !hasCAN;
    hasGPS         = !hasGPS;

    blinkLED(500, 1, 1, 1, 0, 0);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// adjust output frequency
void setFrequencyRPM(long frequencyHz)
{
  static long     lastFrequencyHz = -1;
  static bool     lastCoilType    = false;

  // Only call LEDC API when something actually changed — calling
  // ledc_set_freq/duty every 1 ms while spinning hammers the LEDC
  // driver spinlock and triggers internal FreeRTOS assertions.
  if (frequencyHz == lastFrequencyHz && coilType == lastCoilType)
    return;

  lastFrequencyHz = frequencyHz;
  lastCoilType    = coilType;

  ledc_channel_t activeChannel   = coilType ? LEDC_RPM_COIL_CHANNEL : LEDC_RPM_PIN_CHANNEL;
  ledc_channel_t inactiveChannel = coilType ? LEDC_RPM_PIN_CHANNEL  : LEDC_RPM_COIL_CHANNEL;

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
  static long lastFrequencyHz = -1;

  if (frequencyHz == lastFrequencyHz)
    return;

  lastFrequencyHz = frequencyHz;

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

void incomingHz()                                               // Interrupt service routine for Hall speed sensor
{
  unsigned long presentMicros = micros();
  unsigned long revolutionTime = presentMicros - hallPreviousMicros; // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL)
    return;                                               // debounce — speed can't exceed 60,000 Hz
  dutyCycleIncoming = (60000000UL / revolutionTime) / 60; // calculate frequency in Hz
  hallPreviousMicros = presentMicros;
  lastPulse = millis();
}

void incomingRPMHz()                                            // Interrupt service routine for engine RPM pulse
{
  unsigned long presentMicros = micros();
  unsigned long revolutionTime = presentMicros - rpmPreviousMicros; // works fine with wrap-around of micros()
  if (revolutionTime < 1000UL)
    return;                                                    // debounce
  dutyCycleMotor = (60000000UL / revolutionTime) / 60;        // calculate frequency in Hz
  rpmPreviousMicros = presentMicros;
  lastPulseRPM = millis();
}