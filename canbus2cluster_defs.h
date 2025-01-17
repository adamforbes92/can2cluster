/* Defines */
// Debug statements
#define ChassisCANDebug 0      // if 1, will print CAN 2 (Chassis) messages ** CAN CHANGE THIS **
#define stateDebug 1           // if 1, will use Serial talkback ** CAN CHANGE THIS **
#define selfTest 1             // increase RPM/speed slowly, flash lights.  For debug only, disable on release! ** CAN CHANGE THIS **

// setup
#define hasCoilOutput 1        // is MK2 / use MK2 Output.  Disable if not being used to save power - no point in triggering the relay for something to do... ** CAN CHANGE THIS **
#define hasNeedleSweep 1       // do needle sweep on power up? ** CAN CHANGE THIS **
#define hasGPSSpeed 1          // use GPS module for getting speed - if CAN/gearbox speed sensor isn't available ** CAN CHANGE THIS **

#define needleSweepDelay 5     // delay between next freq.  Increase/decrease to change the sweep time ** CAN CHANGE THIS **
#define useEPCShiftLight 1     // use the EPC output as a shift light ** CAN CHANGE THIS **
#define useEMLShiftLight 1     // use the EML output as a shift light ** CAN CHANGE THIS **
#define shiftLightRate 60      // flash EPC at xx ms.  Decreasing may lead to a 'constant' light because of the human eye... ** CAN CHANGE THIS **

#define maxRRM 455             // max RPM in Hz for the cluster (for needle sweep) ** CAN CHANGE THIS **
#define maxSpeed 500           // max Speed in Hz for the cluster (for needle sweep).  MK3 default is 500.  MK1/MK2 (has cable), default is xxx ** CAN CHANGE THIS **

// pins
#define clusterRPMLimit 7000   // rpm ** CAN CHANGE THIS **
#define clusterSpeedLimit 200  // km/h ** CAN CHANGE THIS **
#define shiftLimit 6000        // set the RPM limit for the shift light ** CAN CHANGE THIS **
#define pinRX_CAN 16           // pin output for SN65HVD230 (CAN_RX)
#define pinTX_CAN 17           // pin output for SN65HVD230 (CAN_TX)
#define pinRX_GPS 14           // pin output for GPS NEO6M (GPS_RX)
#define pinTX_GPS 13           // pin output for GPS NEO6M (GPS_TX)
#define pinCoil 18             // pin output for RPM (MK2/High Output Coil Trigger)
#define pinEPC 19              // pin output for EPC
#define pinEML 21              // pin output for EML
#define pinRPM 22              // pin output for RPM22
#define pinSpeed 23            // pin output for Speed

#define pinPaddleUp 36         // DSG paddle up
#define pinPaddleDown 39       // DSG paddle down
#define pinReverse 26          // pin for relay / reverse
#define pinSpare1 34              // spare 1
#define pinSpare2 32              // spare 2

// Baud Rates
#define baudSerial 115200      // baud rate for debug
#define baudGPS 9600           // baud rate for the GPS device

extern uint16_t vehicleRPM = 1;    // current RPM.  If no CAN, this will catch dividing by zero by the map function
extern int vehicleSpeed = 1;       // current Speed.  If no CAN, this will catch dividing by zero by the map function
extern int calcSpeed = 0;          // temp var for calculating speed
extern int finalFrequencyRPM = 0;
extern int finalFrequencySpeed = 0;
extern int i = 0;

extern bool vehicleEML = false;    // current EML light status
extern bool vehicleEPC = false;    // current EPC light status
extern bool vehicleReverse = false;
extern bool vehiclePark = false;

extern bool boolPadUp = false;    // current EML light status
extern bool boolPadDown = false;    // current EPC light status
extern bool boolSpare1 = false;    // current EML light status
extern bool boolSpare2 = false;    // current EPC light status

extern bool hasError = false;

// define CAN Addresses.  All not req. but here for keepsakes
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
extern void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML, bool boolRPM, bool boolSpeed);