/* Defines */
// Debug statements
#define ChassisCANDebug 0      // if 1, will print CAN 2 (Chassis) messages
#define stateDebug 1           // if 1, will Serial print
#define selfTest 1             // lock needles at mid-point

#define pinRX 16               // pin output for SN65HVD230 (CAN_RX)
#define pinTX 17               // pin output for SN65HVD230 (CAN_TX)
#define pinCoil 18             // pin output for RPM (MK2/High Output Coil Trigger)
#define pinEPC 19              // pin output for EPC
#define pinEML 21              // pin output for EML
#define pinRPM 22              // pin output for RPM
#define pinSpeed 23            // pin output for Speed

#define hasCoilOutput 1        // is MK2 / use MK2 Output.  Disable if not being used to save power...
#define hasNeedleSweep 1       // do needle sweep on power up?

#define needleSweepDelay 250   // delay between next freq.
#define useEPCShiftLight 1     // use the EPC output as a shift light
#define useEMLShiftLight 1     // use the EML output as a shift light
#define shiftLightRate 50      // flash EPC at xx ms

#define maxRRM 300             // max RPM in Hz for the cluster (for needle sweep)
#define maxSpeed 500           // max Speed in Hz for the cluster (for needle sweep)

#define clusterRPMLimit 6800   // rpm
#define clusterSpeedLimit 200  // km/h
#define shiftLimit 6000        // set the RPM limit for the shift light

// Baud Rates
#define baudSerial 115200      // baud rate for debug

extern int vehicleRPM;         // current RPM
extern int vehicleSpeed;       // current Speed
extern bool vehicleEML;        // current EML light status
extern bool vehicleEPC;        // current EPC light status
extern int calcSpeed;          // temp var for calculating speed

// define CAN Addresses
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588

#define MOTOR_FLEX_ID 0x580
#define GRA_ID 0x38A

#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x2A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0

#define HALDEX_ID 0x2C0

extern void basicInit(void);
extern void canInit(void);
extern void onBodyRX(void);
extern void needleSweep(void);
extern void setupPins(void);
extern void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML);