#ifndef CAN2CLUSTER_I2C_H
#define CAN2CLUSTER_I2C_H

#include "can2cluster_defs.h"

// ----------------------------------------------------------------------------
// I2C bus - S3 board only.
//
// The only thing on the bus is the MCP4725 DAC (U5, 0x60). Its output sets the
// current the op-amp sink (U13 + Q1) pulls through the coolant gauge on
// COOLANT_OUT (J1 pin 16): 0 = no current, 4095 = full current.
//
// The DevKit board has no I2C parts - every function here does nothing there.
// ----------------------------------------------------------------------------

void i2cInit(void);                   // S3: start the I2C bus, look for the MCP4725 and set the gauge to 0
void mcp4725Write(uint16_t value12);  // S3: write 0..4095 to the coolant DAC
uint16_t coolantMaxOut(void);         // full-scale coolant output: 4095 (S3 DAC) / 1023 (DevKit PWM)
void i2cSerialDiag(void);             // 1 Hz [I2C] Serial line (only when serialDebugI2C is 1)

#endif // CAN2CLUSTER_I2C_H
