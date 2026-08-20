#include "can2cluster_i2c.h"
#include "can2cluster_buttons.h"
#include <Wire.h>

// ----------------------------------------------------------------------------
// TCA9554 register map (8-bit; NOT the 16-bit TCA9555)
// ----------------------------------------------------------------------------
namespace
{
constexpr uint8_t TCA_REG_INPUT  = 0x00;
constexpr uint8_t TCA_REG_OUTPUT = 0x01;
constexpr uint8_t TCA_REG_CONFIG = 0x03; // 1 = input, 0 = output

// Port direction: paddle bits are inputs, the rest outputs.
constexpr uint8_t TCA_CONFIG = (1 << TCA_BIT_PADUP) | (1 << TCA_BIT_PADDOWN);

// Output shadow — the last byte written to the expander's output port 0.
// Actuators are active-HIGH (bit set = driver stage energised), matching the
// old board's ULN2003 / MOSFET logic levels.
uint8_t tcaOutputShadow = 0;

// Paddle edge-detection state (previous raw input levels, active-LOW).
bool paddleUpPrev = true;   // idle = HIGH (pulled up)
bool paddleDownPrev = true;

bool setTcaBit(uint8_t bit, bool on)
{
  uint8_t next = on ? (tcaOutputShadow | (1 << bit)) : (tcaOutputShadow & ~(1 << bit));
  if (next == tcaOutputShadow)
    return true; // no change — skip the bus transaction
  tcaOutputShadow = next;
  return tca9554FlushOutputs();
}
} // namespace

// ----------------------------------------------------------------------------
// Board detection
// ----------------------------------------------------------------------------
void detectBoard()
{
  pinMode(pinBoardSense, INPUT_PULLUP);
  delayMicroseconds(50); // let the internal pull settle before sampling
  isNewBoard = (digitalRead(pinBoardSense) == LOW);
  DEBUG_I2C("Board sense (GPIO%d) = %s -> %s board", pinBoardSense,
            isNewBoard ? "LOW" : "HIGH", isNewBoard ? "NEW (I2C)" : "OLD (GPIO)");
}

// ----------------------------------------------------------------------------
// TCA interrupt — just flag activity so we poll the expander promptly. Reading
// I2C from an ISR is illegal, so the actual read happens in tcaPollPaddles().
// ----------------------------------------------------------------------------
volatile uint32_t tcaIntCount = 0;

void IRAM_ATTR tcaIntISR()
{
  tcaIntCount++;
}

// ----------------------------------------------------------------------------
// Bus + peripheral setup
// ----------------------------------------------------------------------------
void i2cInit()
{
  if (!isNewBoard)
  {
    DEBUG_I2C("Old board — I2C peripherals not present, skipping bus init");
    return;
  }

  Wire.begin(pinI2C_SDA, pinI2C_SCL, I2C_FREQ_HZ);
  DEBUG_I2C("Wire.begin(SDA=%d, SCL=%d, %lu Hz)", pinI2C_SDA, pinI2C_SCL, (unsigned long)I2C_FREQ_HZ);

  i2cScan();

  if (i2cTcaPresent)
    tca9554Init();
  else
    DEBUG_I2C("WARNING: TCA9554 (0x%02X) not found", TCA9554_ADDR);

  if (i2cMcpPresent)
    mcp4725Write(0); // gauge off at boot
  else
    DEBUG_I2C("WARNING: MCP4725 (0x%02X) not found", MCP4725_ADDR);

  // TCA INT -> GPIO34 (active-low, open-drain): watch for paddle activity.
  pinMode(pinTCA_INT, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinTCA_INT), tcaIntISR, FALLING);
}

void i2cScan()
{
  if (!isNewBoard)
  {
    i2cMcpPresent = false;
    i2cTcaPresent = false;
    return;
  }

  Wire.beginTransmission(MCP4725_ADDR);
  i2cMcpPresent = (Wire.endTransmission() == 0);

  Wire.beginTransmission(TCA9554_ADDR);
  i2cTcaPresent = (Wire.endTransmission() == 0);

  DEBUG_I2C("Scan: MCP4725=%s TCA9554=%s",
            i2cMcpPresent ? "OK" : "--", i2cTcaPresent ? "OK" : "--");
}

// ----------------------------------------------------------------------------
// MCP4725 coolant DAC (12-bit, fast-mode write)
// ----------------------------------------------------------------------------
void mcp4725Write(uint16_t value12)
{
  if (!isNewBoard || !i2cMcpPresent)
    return;
  if (value12 > MCP4725_MAX)
    value12 = MCP4725_MAX;

  Wire.beginTransmission(MCP4725_ADDR);
  Wire.write((value12 >> 8) & 0x0F); // C2=C1=0 (fast mode), PD=0, D11..D8
  Wire.write(value12 & 0xFF);        // D7..D0
  Wire.endTransmission();
}

// ----------------------------------------------------------------------------
// TCA9554 I/O expander
// ----------------------------------------------------------------------------
bool tca9554Init()
{
  tcaOutputShadow = 0;

  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(TCA_REG_CONFIG);
  Wire.write(TCA_CONFIG); // port directions
  if (Wire.endTransmission() != 0)
  {
    DEBUG_I2C("TCA9554 config write failed");
    return false;
  }

  bool ok = tca9554FlushOutputs(); // drive all outputs low
  DEBUG_I2C("TCA9554 init %s (config=0x%02X)", ok ? "OK" : "FAILED", TCA_CONFIG);
  return ok;
}

bool tca9554FlushOutputs()
{
  if (!isNewBoard || !i2cTcaPresent)
    return false;
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(TCA_REG_OUTPUT);
  Wire.write(tcaOutputShadow);
  return (Wire.endTransmission() == 0);
}

bool tca9554ReadInputs(uint8_t *port0)
{
  if (!isNewBoard || !i2cTcaPresent)
    return false;
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(TCA_REG_INPUT);
  if (Wire.endTransmission(false) != 0) // repeated start
    return false;
  if (Wire.requestFrom(TCA9554_ADDR, (uint8_t)1) != 1)
    return false;
  *port0 = Wire.read();
  return true;
}

// Read the expander and translate paddle edges into the same one-shot events the
// OneButton click callbacks produce on the old board.
void tcaPollPaddles()
{
  uint8_t port0;
  if (!tca9554ReadInputs(&port0))
    return;
  tcaInputShadow = port0;

  bool upNow = (port0 >> TCA_BIT_PADUP) & 0x01;     // active-LOW: LOW = pressed
  bool downNow = (port0 >> TCA_BIT_PADDOWN) & 0x01;

  if (paddleUpPrev && !upNow) // HIGH -> LOW = fresh press
    padUpFunc();
  if (paddleDownPrev && !downNow)
    padDownFunc();

  paddleUpPrev = upNow;
  paddleDownPrev = downNow;
}

// ----------------------------------------------------------------------------
// Board-abstracted actuators
// ----------------------------------------------------------------------------
void driveReverse(bool on)
{
  if (isNewBoard)
    setTcaBit(TCA_BIT_REVERSE, on);
  else
    digitalWrite(pinReverse, on ? HIGH : LOW);
}

void driveEPC(bool on)
{
  if (isNewBoard)
    setTcaBit(TCA_BIT_EPC, on);
  else
    digitalWrite(pinEPC, on ? HIGH : LOW);
}

void driveEML(bool on)
{
  if (isNewBoard)
    setTcaBit(TCA_BIT_EML, on);
  else
    digitalWrite(pinEML, on ? HIGH : LOW);
}

// ----------------------------------------------------------------------------
// Coolant transport
// ----------------------------------------------------------------------------
uint16_t coolantMaxOut()
{
  return isNewBoard ? MCP4725_MAX : 1023;
}

// New-board coolant path: mirrors the old LEDC idiot-light logic but writes the
// MCP4725 DAC instead. Only touches the bus on a value change.
void updateCoolantDAC()
{
  if (coolantOutput == 0) // gauge disabled
  {
    if (coolantAppliedDuty != 0)
    {
      coolantAppliedDuty = 0;
      mcp4725Write(0);
    }
    return;
  }

  uint16_t duty;
  if (coolantCalMode)
    duty = coolantCalDutyNow;
  else
    duty = (vehicleCoolantTemp >= coolantWarnTemp) ? MCP4725_MAX : 0;
  if (duty > MCP4725_MAX)
    duty = MCP4725_MAX;
  coolantAppliedDuty = duty;

  static uint16_t lastDuty = 0xFFFF;
  if (duty != lastDuty)
  {
    lastDuty = duty;
    mcp4725Write(duty);
  }
}

// ----------------------------------------------------------------------------
// Diagnostics — 1 Hz [I2C] telemetry block
// ----------------------------------------------------------------------------
void i2cSerialDiag()
{
#if serialDebugI2C
  if (!isNewBoard)
  {
    DEBUG_I2C("Old board (no I2C peripherals)");
    return;
  }
  DEBUG_I2C("MCP4725=%s TCA9554=%s | coolantDAC=%u | TCA out=0x%02X in=0x%02X | INT=%lu",
            i2cMcpPresent ? "OK" : "--",
            i2cTcaPresent ? "OK" : "--",
            coolantAppliedDuty,
            tcaOutputShadow,
            tcaInputShadow,
            (unsigned long)tcaIntCount);
#endif
}
