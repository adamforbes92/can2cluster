#include "can2cluster_io.h"
#include "can2cluster_gps.h"
#include "can2cluster_i2c.h"
#include <driver/ledc.h>

// Windowed frequency capture — shared by the hall, VR and RPM inputs. Deriving Hz
// from a single edge-to-edge period turns tone-wheel / tooth-spacing jitter straight
// into a jumpy reading, so instead each ISR accumulates the summed edge intervals and
// the interval count. The task reads-and-clears the pair once per loop, so
// freq = count / summedInterval is a true average over the window.
//
// File-scope (not function-local statics): C++ function-local statics with non-constant
// initialisers call __cxa_guard_acquire() -> pthread_mutex_lock() -> xSemaphoreTake()
// on their first invocation, which asserts inside FreeRTOS when triggered from an ISR
// context (xQueueSemaphoreTake assert in queue.c).
struct PulseWindow
{
  volatile uint32_t accumUs;    // summed edge-to-edge intervals (us) this window
  volatile uint32_t count;      // number of intervals summed into accumUs
  volatile uint32_t lastEdgeUs; // micros() of the previous accepted edge (kept across windows)
};

static PulseWindow hallPulse = {0, 0, 0}; // vehicle hall speed input
static PulseWindow vrPulse = {0, 0, 0};   // variable-reluctance speed input
static PulseWindow rpmPulse = {0, 0, 0};  // engine RPM input

// Reject edges closer together than this. Ignition-coil EMI couples in as
// sub-millisecond bursts, while a real input edge tops out around 230 Hz (~4.3 ms),
// so anything faster than ~3 ms (333 Hz) can't be a genuine pulse and is dropped.
static const uint32_t PULSE_MIN_INTERVAL_US = 3000;

// Guards the read-and-clear of the pulse windows against the ISRs. portMUX is the
// FreeRTOS/ESP32-safe primitive; a global noInterrupts() would starve the WiFi radio.
static portMUX_TYPE pulseMux = portMUX_INITIALIZER_UNLOCKED;

// Accumulate one edge into a window. Returns true when the edge was accepted (it
// advanced the average), false when it merely seeded the window or was rejected as a
// glitch. lastEdgeUs is kept on a glitch so the next real edge still measures from the
// last good edge.
static inline bool pulseEdge(PulseWindow &w, uint32_t now)
{
  uint32_t last = w.lastEdgeUs;
  if (last == 0)
  {
    w.lastEdgeUs = now; // seed on the first pulse only
    return false;
  }
  uint32_t interval = now - last;
  if (interval < PULSE_MIN_INTERVAL_US)
    return false; // coil EMI / bounce: ignore, keep the last good edge
  w.accumUs += interval;
  w.count++;
  w.lastEdgeUs = now;
  return true;
}

// Read-and-clear a window, returning its averaged frequency in Hz, or -1 when no fresh
// edges arrived (caller should hold the previous value). lastEdgeUs is left intact so
// the next window's first interval bridges from this window's last edge.
static float readPulseHz(PulseWindow &w)
{
  portENTER_CRITICAL(&pulseMux);
  uint32_t count = w.count;
  uint32_t accum = w.accumUs;
  w.count = 0;
  w.accumUs = 0;
  portEXIT_CRITICAL(&pulseMux);
  if (count >= 1 && accum > 0)
    return (float)count * 1000000.0f / (float)accum;
  return -1.0f;
}

// Full reset (including the edge reference) — used on timeout so a stale lastEdgeUs
// can't inject one bogus huge-interval reading when the input resumes.
static void resetPulseWindow(PulseWindow &w)
{
  portENTER_CRITICAL(&pulseMux);
  w.accumUs = 0;
  w.count = 0;
  w.lastEdgeUs = 0;
  portEXIT_CRITICAL(&pulseMux);
}

float readHallHz() { return readPulseHz(hallPulse); }
float readVRHz() { return readPulseHz(vrPulse); }
float readRPMHz() { return readPulseHz(rpmPulse); }
void resetHallPulseCounter() { resetPulseWindow(hallPulse); }
void resetVRPulseCounter() { resetPulseWindow(vrPulse); }
void resetRPMPulseCounter() { resetPulseWindow(rpmPulse); }

namespace
{
constexpr ledc_mode_t LEDC_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_bit_t LEDC_RESOLUTION = LEDC_TIMER_10_BIT;

constexpr ledc_timer_t LEDC_SPEED_TIMER = LEDC_TIMER_0;
constexpr ledc_channel_t LEDC_SPEED_CHANNEL = LEDC_CHANNEL_0;

constexpr ledc_timer_t LEDC_RPM_TIMER = LEDC_TIMER_1;
constexpr ledc_channel_t LEDC_RPM_COIL_CHANNEL = LEDC_CHANNEL_1;
constexpr ledc_channel_t LEDC_RPM_PIN_CHANNEL = LEDC_CHANNEL_2;

// Coolant gauge shares the EML/EPC ULN2003 output; its own timer/channel so it
// can run a fixed PWM frequency while the RPM/Speed timers do their own thing.
constexpr ledc_timer_t LEDC_COOLANT_TIMER = LEDC_TIMER_2;
constexpr ledc_channel_t LEDC_COOLANT_CHANNEL = LEDC_CHANNEL_3;
// Run the coolant PWM on the ESP32's independent high-speed LEDC block so
// retuning it never disturbs the shared low-speed clock the RPM/Speed timers
// use (changing one low-speed timer's clock source affects them all).
constexpr ledc_mode_t LEDC_COOLANT_MODE = LEDC_HIGH_SPEED_MODE;
constexpr uint32_t LEDC_COOLANT_MAX_DUTY = 1023; // 10-bit resolution

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
#if serialDebug || serialDebugWifi || serialDebugEEP || serialDebugGPS || ChassisCANDebug || serialDebugPaddles || serialDebugIO || serialDebugDSG || serialDebugCAN || serialDebugI2C
  Serial.begin(baudSerial);
  delay(500);
#endif
  DEBUG("[Init] CAN-BUS to Cluster Initialising...");

  // Detect the board revision and bring up the I2C peripherals before anything
  // reads isNewBoard (pin setup, coolant driver, paddle polling).
  detectBoard();
  i2cInit();

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
  // On the new board EML/EPC/Reverse live on the TCA9554 and GPIO21/19/26 are
  // repurposed (I2C bus + chassis CAN) — never drive them as GPIO there.
  if (!isNewBoard)
  {
    pinMode(pinEML, OUTPUT);     // for engine management light output
    pinMode(pinEPC, OUTPUT);     // for electronic pedal control output
    pinMode(pinReverse, OUTPUT); // for reverse MOSFET output (5A max!)
  }

  pinMode(pinCoil, OUTPUT); // for high-voltage RPM (can be turned on/off in WiFi so always enable regardless)
  pinMode(pinRPM, OUTPUT);  // for standard square wave RPM
  pinMode(pinRpmPulse, INPUT);
  pinMode(pinVR, INPUT); // VR speed sensor input (external pull-up to 3.3V)

  // Hold generated outputs low until an active frequency is requested.
  digitalWrite(pinCoil, LOW);
  digitalWrite(pinRPM, LOW);
  digitalWrite(pinSpeed, LOW);

  setupLedcOutputs();

  attachInterrupt(digitalPinToInterrupt(pinHallSensor), incomingHz, FALLING); // setup interrupt to toggle pin on change
  attachInterrupt(digitalPinToInterrupt(pinVR), incomingVRHz, FALLING); // setup interrupt for VR speed sensor input
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
    driveEPC(false);
  }
  if (blinkState.boolEML)
  {
    driveEML(false);
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
      driveEPC(blinkState.outputState);
    }
    if (blinkState.boolEML)
    {
      driveEML(blinkState.outputState);
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
      // Sequence complete - turn off all outputs (but never the pin the
      // coolant gauge owns on the old board, or we'd punch a hole in its PWM).
      if (isNewBoard || coolantOutput != 2)
        driveEPC(false);
      if (isNewBoard || coolantOutput != 1)
        driveEML(false);
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

// Interpolate a PWM duty (0-1023) for a coolant temperature from the
// calibration table. Points are stored sorted ascending by temperature; the
// curve is clamped (held flat) beyond the first/last captured point.
uint16_t coolantDutyForTemp(int16_t tempC)
{
  if (coolantCalCount == 0)
    return 0;
  if (coolantCalCount == 1)
    return coolantCalDuty[0];

  if (tempC <= coolantCalTemp[0])
    return coolantCalDuty[0];
  if (tempC >= coolantCalTemp[coolantCalCount - 1])
    return coolantCalDuty[coolantCalCount - 1];

  for (uint8_t i = 1; i < coolantCalCount; i++)
  {
    if (tempC <= coolantCalTemp[i])
    {
      int32_t t0 = coolantCalTemp[i - 1];
      int32_t t1 = coolantCalTemp[i];
      int32_t d0 = coolantCalDuty[i - 1];
      int32_t d1 = coolantCalDuty[i];
      int32_t span = t1 - t0;
      if (span == 0)
        return static_cast<uint16_t>(d1);
      return static_cast<uint16_t>(d0 + (d1 - d0) * (tempC - t0) / span);
    }
  }
  return coolantCalDuty[coolantCalCount - 1];
}

// Attach the coolant LEDC channel to the chosen EML/EPC pin, or detach and
// return the pin to plain GPIO output so outputControlTask can drive it as a
// normal warning light again. Only reconfigures when the target pin changes.
void applyCoolantOutput()
{
  static int coolantActivePin = -1;

  int desired = -1;
  if (coolantOutput == 1)
    desired = pinEML;
  else if (coolantOutput == 2)
    desired = pinEPC;

  // A diagnostic test on the coolant's pin is a hard override: drop the LEDC
  // channel so outputControlTask can drive the pin directly (direct short).
  if ((desired == pinEML && testEML) || (desired == pinEPC && testEPC))
    desired = -1;

  if (desired == coolantActivePin)
    return;

  // Detach from the previous pin and restore normal GPIO control.
  if (coolantActivePin >= 0)
  {
    ledc_stop(LEDC_COOLANT_MODE, LEDC_COOLANT_CHANNEL, 0);
    pinMode(coolantActivePin, OUTPUT);
    digitalWrite(coolantActivePin, LOW);
  }

  if (desired >= 0)
  {
    ledc_timer_config_t coolantTimerConfig = {};
    coolantTimerConfig.speed_mode = LEDC_COOLANT_MODE;
    coolantTimerConfig.timer_num = LEDC_COOLANT_TIMER;
    coolantTimerConfig.duty_resolution = LEDC_RESOLUTION;
    coolantTimerConfig.freq_hz = coolantPwmFreq > 0 ? coolantPwmFreq : 100;
    coolantTimerConfig.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&coolantTimerConfig);

    ledc_channel_config_t coolantChannelConfig = {};
    coolantChannelConfig.gpio_num = desired;
    coolantChannelConfig.speed_mode = LEDC_COOLANT_MODE;
    coolantChannelConfig.channel = LEDC_COOLANT_CHANNEL;
    coolantChannelConfig.intr_type = LEDC_INTR_DISABLE;
    coolantChannelConfig.timer_sel = LEDC_COOLANT_TIMER;
    coolantChannelConfig.duty = LEDC_DUTY_OFF;
    coolantChannelConfig.hpoint = 0;
    ledc_channel_config(&coolantChannelConfig);
  }

  coolantActivePin = desired;
}

// Drive the coolant gauge each output cycle: while calibrating, hold the jog
// duty so the needle can be read; otherwise map the live CAN temperature
// through the calibration table. Only touches the LEDC driver on a change.
void updateCoolantOutput()
{
  // New board: coolant is a dedicated MCP4725 DAC, not a shared EML/EPC pin.
  if (isNewBoard)
  {
    updateCoolantDAC();
    return;
  }

  applyCoolantOutput();

  if (coolantOutput == 0)
    return;

  // A diagnostic test overrides our pin — LEDC is detached, leave it alone so
  // outputControlTask's direct drive wins.
  if ((coolantOutput == 1 && testEML) || (coolantOutput == 2 && testEPC))
    return;

  static uint32_t lastFreq = 0;
  if (coolantPwmFreq != lastFreq)
  {
    lastFreq = coolantPwmFreq;
    ledc_set_freq(LEDC_COOLANT_MODE, LEDC_COOLANT_TIMER, coolantPwmFreq > 0 ? coolantPwmFreq : 100);
  }

  // Idiot-light gauge: while calibrating hold the jog duty; otherwise peg the
  // needle fully (tripping the cluster's warning lamp) at or above the warning
  // temperature, and stay off below it.
  uint16_t duty;
  if (coolantCalMode)
    duty = coolantCalDutyNow;
  else
    duty = (vehicleCoolantTemp >= coolantWarnTemp) ? LEDC_COOLANT_MAX_DUTY : 0;
  if (duty > LEDC_COOLANT_MAX_DUTY)
    duty = LEDC_COOLANT_MAX_DUTY;
  coolantAppliedDuty = duty;

  static uint16_t lastDuty = 0xFFFF;
  if (duty != lastDuty)
  {
    lastDuty = duty;
    ledc_set_duty(LEDC_COOLANT_MODE, LEDC_COOLANT_CHANNEL, duty);
    ledc_update_duty(LEDC_COOLANT_MODE, LEDC_COOLANT_CHANNEL);
  }
}

void incomingHz()                                               // Interrupt service routine for Hall speed sensor
{
  if (pulseEdge(hallPulse, micros()))
    lastPulse = millis();
}

void incomingVRHz()                                             // Interrupt service routine for variable-reluctance speed sensor
{
  if (pulseEdge(vrPulse, micros()))
    lastPulseVR = millis();
}

void incomingRPMHz()                                            // Interrupt service routine for engine RPM pulse
{
  if (pulseEdge(rpmPulse, micros()))
    lastPulseRPM = millis();
}