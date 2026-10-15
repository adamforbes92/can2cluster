#include "can2cluster_i2c.h"
#include <Wire.h>

// Start the I2C bus and check the coolant DAC is there (S3 board only)
void i2cInit()
{
#if BOARD_S3
  DEBUG_I2C("Setting up I2C...");
  Wire.begin(pinI2C_SDA, pinI2C_SCL, I2C_FREQ_HZ);

  // Ask the MCP4725 for an acknowledge - 0 back means it answered
  Wire.beginTransmission(MCP4725_ADDR);
  if (Wire.endTransmission() == 0)
  {
    i2cMcpPresent = true;
  }
  else
  {
    i2cMcpPresent = false;
  }

  if (i2cMcpPresent)
  {
    mcp4725Write(0); // gauge off at boot
    DEBUG_I2C("Set up I2C! MCP4725 (0x%02X) found", MCP4725_ADDR);
  }
  else
  {
    DEBUG_I2C("WARNING: MCP4725 (0x%02X) not found - coolant gauge disabled", MCP4725_ADDR);
  }
#endif
}

// Write a 12-bit value (0..4095) to the coolant DAC (S3 board only)
void mcp4725Write(uint16_t value12)
{
#if BOARD_S3
  if (!i2cMcpPresent)
  {
    return; // nothing to talk to
  }
  if (value12 > MCP4725_MAX)
  {
    value12 = MCP4725_MAX;
  }

  uint8_t highByte = (value12 >> 8) & 0x0F; // fast-mode write: top 4 bits zero (normal mode), then D11..D8
  uint8_t lowByte = value12 & 0xFF;         // D7..D0

  Wire.beginTransmission(MCP4725_ADDR);
  Wire.write(highByte);
  Wire.write(lowByte);
  Wire.endTransmission();
#endif
}

// Biggest value the coolant output takes: the S3 DAC is 12-bit, the DevKit PWM is 10-bit
uint16_t coolantMaxOut()
{
#if BOARD_S3
  return MCP4725_MAX; // 4095
#else
  return 1023;
#endif
}

// Once-a-second Serial line for the I2C side (S3 only, serialDebugI2C = 1)
void i2cSerialDiag()
{
#if serialDebugI2C && BOARD_S3
  if (i2cMcpPresent)
  {
    DEBUG_I2C("MCP4725 OK | coolant DAC = %u | board ID = %u mV", coolantAppliedDuty, boardVersionMilliVolts);
  }
  else
  {
    DEBUG_I2C("MCP4725 missing | board ID = %u mV", boardVersionMilliVolts);
  }
#endif
}
