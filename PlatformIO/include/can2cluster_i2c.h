#ifndef CAN2CLUSTER_I2C_H
#define CAN2CLUSTER_I2C_H

#include "can2cluster_defs.h"

// ----------------------------------------------------------------------------
// Board revision + I2C peripheral layer.
//
// The "new" board grounds pinBoardSense (GPIO32) and moves several signals onto
// the I2C bus (SDA=21, SCL=19):
//   - Coolant gauge   -> MCP4725 DAC (0x60) -> op-amp -> BSS138 gate
//   - Reverse/EPC/EML -> TCA9554 expander (0x20) port outputs
//   - DSG paddles     -> TCA9554 port inputs, with INT wired to GPIO34
//
// On the "old" board none of this exists: EML/EPC/Reverse are plain GPIO and the
// coolant gauge is an LEDC PWM output. Everything here is a no-op / GPIO
// passthrough when isNewBoard == false, so a single binary drives both boards.
// ----------------------------------------------------------------------------

// Board detection ------------------------------------------------------------
void detectBoard(void); // read pinBoardSense; set isNewBoard (LOW = new board)

// Bus + peripheral setup -----------------------------------------------------
void i2cInit(void);  // Wire.begin + configure MCP4725/TCA9554 + attach TCA INT (new board only)
void i2cScan(void);  // refresh i2cMcpPresent / i2cTcaPresent by pinging each address

// MCP4725 coolant DAC --------------------------------------------------------
void mcp4725Write(uint16_t value12); // write 0..4095 (fast mode)

// TCA9554 I/O expander -------------------------------------------------------
bool tca9554Init(void);              // configure port directions; returns true on ACK
bool tca9554FlushOutputs(void);      // push the output shadow to the device
bool tca9554ReadInputs(uint8_t *port0); // read input port into *port0
void tcaPollPaddles(void);           // edge-detect TCA paddle inputs -> padUpFunc/padDownFunc

// Board-abstracted actuators (GPIO on old board, TCA bit on new board) -------
void driveReverse(bool on);
void driveEPC(bool on);
void driveEML(bool on);

// Coolant transport ----------------------------------------------------------
uint16_t coolantMaxOut(void); // full-scale output value: 4095 (DAC) / 1023 (LEDC)
void updateCoolantDAC(void);  // new-board coolant path: compute + write the DAC

// Diagnostics ----------------------------------------------------------------
void i2cSerialDiag(void); // 1 Hz [I2C] telemetry block (gated by serialDebugI2C)

// TCA interrupt (paddle awareness) -------------------------------------------
extern volatile uint32_t tcaIntCount; // incremented by the TCA INT ISR
void IRAM_ATTR tcaIntISR(void);

#endif // CAN2CLUSTER_I2C_H
