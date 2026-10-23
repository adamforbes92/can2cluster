#include "can2cluster_defs.h"

// setup - cluster RPM & speed limits
uint16_t clusterRPMLimit = 7000;
uint16_t shiftLimit = 6000;
uint8_t shiftFlashes = 3;
uint8_t sweepSpeed = 18;
uint16_t maxSpeed = 200;         // speedo dial top (km/h or mph) - the needle sweep goes up to here
uint16_t maxRPM = 230;
uint16_t maxFreqHall = 200;
uint16_t maxFreqVR = 200;
bool useEPCShiftLight = false;
bool useEMLShiftLight = false;

// setup - step changes (for needle sweep, used as duration multipliers in time-based formula)
uint16_t stepRPM = 12;
uint16_t stepSpeed = 10;

// existing variables that were previously defined in header:
uint8_t vehicleCoolantTemp = 0;
uint16_t vehicleRPMCAN = 0;
uint16_t vehicleRPM = 0;
uint16_t vehicleSpeed = 0;
uint16_t calcSpeed = 0;
long tempSpeed = 0;
long tempRPM = 0;
long frequencyRPM = 20;
long frequencySpeed = 20;

double ecuSpeed = 0;
double dsgSpeed = 0;
double gpsSpeed = 0;
double absSpeed = 0;
double hallSpeed = 0;
double vrSpeed = 0;

bool rpmTrigger = true;
bool speedTrigger = true;

uint8_t gear = 0;
uint8_t lever = 0;
uint8_t gear_raw = 0;
uint8_t lever_raw = 0;
float dsgGearRatio[7] = {1.0f, 3.462f, 2.050f, 1.300f, 0.902f, 0.914f, 0.756f}; // index 0 unused
float dsgFinalDrive14 = 4.118f;
float dsgFinalDrive56 = 3.043f;
float dsgTireCirc = 1.885f; // PI * 0.6m rolling diameter
uint32_t lastMillis = 0;
uint32_t lastMillis2 = 0;
uint32_t lastCAN = 0;
volatile unsigned long lastPulse = 0;
volatile unsigned long dutyCycleIncoming = 0;
volatile unsigned long lastPulseVR = 0;
volatile unsigned long dutyCycleIncomingVR = 0;
volatile unsigned long dutyCycleMotor = 0;
volatile unsigned long lastPulseRPM = 0;

bool vehicleEML = false;
bool vehicleEPC = false;
bool vehiclePark = false;
bool vehicleNeutral = false;
bool vehicleReverse = false;
bool vehicleOilPressure = false;
bool vehicleBattLight = false;
uint8_t GRA_counter = 0; // PQ GRA_Neu message counter, 0-15
uint8_t GRA_crc = 0;     // PQ GRA_Neu checksum of the last frame sent

// DSG paddles. "volatile" = set by one task and read by another (or the web
// page), so always re-read it rather than trusting a copy.
bool boolPadUp = false;                       // web page feedback: paddle + pulled in the last 2 s
bool boolPadDown = false;                     // web page feedback: paddle - pulled in the last 2 s
volatile bool isPaddleUpHeld = false;         // paddle + held right now
volatile bool isPaddleDownHeld = false;       // paddle - held right now
volatile uint8_t paddleTestCmd = MQB_PADDLE_NONE; // web page test button: nothing to test yet
volatile uint8_t paddleCmdSent = MQB_PADDLE_NONE; // paddle bits going out right now
volatile uint32_t mqbPaddleOtherSenderMs = 0; // 0 = never seen another 0x3DD sender
volatile uint32_t graPaddleOtherSenderMs = 0; // 0 = never seen another 0x38A sender

// MQB gearbox read-back for the web page. 0xFF = frame not seen yet.
volatile uint8_t mqbLeverPosition = 0xFF;     // lever position from 0x0AF
volatile uint8_t mqbTipState = 0xFF;          // gearbox tip mode from 0x0B1
volatile uint8_t mqbGearboxWarning = 0xFF;    // gearbox warning from 0x394

bool useHall = false;
bool useVR = false;
bool useECU = false;
bool useDSG = false;
bool useGPS = false;
bool useABS = false;
bool useTP20 = false;
bool useUDS  = false;
bool useHallRPM = false;
bool coilType = true;
bool useMPH = false;

bool hasError = false;
bool triggerLED = false;
bool diagTest = false;
bool hasNeedleSweep = false;
bool hasCAN = false;

bool hasGPS = false;
bool gpsUnavailable = false;
bool gpsError = false;
uint8_t gpsUpdateRateHz = 1;
TaskHandle_t gpsTaskHandle = NULL;
TaskHandle_t updateSpeedHandle = NULL;
TaskHandle_t updateRPMHandle = NULL;
bool tempNeedleSweep = false;          // web page asked for a test sweep
volatile bool isNeedleSweeping = false; // no sweep running yet
bool testSpeedo = false;
bool testRPM = false;
bool useAftermarket = false;
uint32_t aftermarketSpeedID = 0x200;
uint8_t aftermarketSpeedLowByte = 0;
uint8_t aftermarketSpeedHighByte = 1;
bool aftermarketSpeedLittleEndian = true;
float aftermarketSpeedScale = 1.0f;
int16_t aftermarketSpeedOffset = 0;
double aftermarketSpeed = 0;
bool broadcastSpeedEnabled = false;
uint32_t broadcastSpeedID = MOTOR2_ID;
uint8_t broadcastSpeedDLC = 8;
uint8_t broadcastSpeedLowByte = 3;
uint8_t broadcastSpeedHighByte = 2;
bool broadcastSpeedLittleEndian = false;
float broadcastSpeedScale = 1.0f;
int16_t broadcastSpeedOffset = 0;
uint16_t broadcastSpeedValue = 0;
uint8_t broadcastSpeedData[8] = {0, 0, 0, 0, 0, 0, 0, 0};
bool tempShiftLight = false;
bool testEML = false;
bool testEPC = false;
bool testReverse = false;
String dsgParkMode = "None";  // DSG Park behavior: "None", "EML", or "EPC"
String paddleOutput = PADDLE_OUTPUT_DEFAULT; // which paddle frame(s) to send

// MK4 cruise stalk. "volatile" = set by the paddle task and read by the web page.
bool useCruiseStalk = false;                 // pins 10/11/12 are paddles + hall speed until switched on the web page
volatile bool isCruiseMainOn = false;        // main switch off
volatile bool isCruiseSetPressed = false;    // SET not pressed
volatile bool isCruiseResumePressed = false; // RESUME not pressed
volatile uint8_t cruiseButtonsByte = 0;      // main switch off, no cancel
volatile uint8_t cruiseSetResumeBits = 0;    // no SET / RESUME

// PQ gateway frames
bool useGatewayFrames = false;                 // off until switched on on the web page
volatile uint32_t bsgLastOtherSenderMs = 0;    // 0 = never seen another 0x570 sender
volatile uint32_t gateKomf1OtherSenderMs = 0;  // 0 = never seen another 0x390 sender
volatile uint32_t diagnose1OtherSenderMs = 0;  // 0 = never seen another 0x7D0 sender
volatile uint32_t clusterKm = 0;               // no mileage from the cluster yet
volatile uint32_t clusterKmSeenMs = 0;         // 0 = never seen 0x520

// Coolant temperature gauge
uint8_t coolantOutput = COOLANT_OFF; // where the gauge is driven from (see COOLANT_xx in _defs.h)
uint32_t coolantPwmFreq = 10000; // Hz (fixed PWM carrier, DevKit only)
uint8_t coolantWarnTemp = 120;   // deg C warning threshold (gauge pegged at/above this)
uint8_t coolantCalCount = 0;
int16_t coolantCalTemp[COOLANT_CAL_MAX] = {0};
uint16_t coolantCalDuty[COOLANT_CAL_MAX] = {0};
bool coolantCalMode = false;
uint16_t coolantCalDutyNow = 0;
uint16_t coolantAppliedDuty = 0;

// S3 board state (stay at these values on the DevKit)
bool i2cMcpPresent = false;         // MCP4725 coolant DAC found on the I2C bus
uint16_t boardVersionMilliVolts = 0; // BOARD_VER pin voltage read at boot

// Blink state tracking for non-blocking LED operations
BlinkState blinkState = {
  false,      // active
  0,          // flashCount
  0,          // currentFlash
  0,          // duration
  0,          // lastToggleTime
  false,      // outputState
  false,      // boolEPC
  false,      // boolEML
  false,      // boolRPM
  false       // boolSpeed
};

uint32_t stackShowState = 0;
uint32_t stackUpdateLabels = 0;
uint32_t stackWriteEEP = 0;

uint32_t stackbroadcastPaddles = 0;
uint32_t stackbroadcastSpeed = 0;
uint32_t stackparseGPS = 0;
uint32_t stackparseDSG = 0;

uint32_t stackupdateSpeed = 0;
uint32_t stackupdateRPM = 0;
uint32_t stackshiftLight = 0;
uint32_t stackcheckError = 0;

uint16_t bool_NeedleSweep = 0;
uint16_t int16_sweepSpeed = 0;
uint16_t int16_stepSpeed = 0;
uint16_t int16_stepRPM = 0;
uint16_t bool_testSpeedo = 0;
uint16_t int16_tempSpeed = 0;
uint16_t bool_useDSG = 0;
uint16_t bool_useGPS = 0;
uint16_t bool_useHall = 0;
uint16_t bool_useABS = 0;
uint16_t bool_testRPM = 0;

uint16_t bool_positiveOffset = 0;
uint16_t int16_speedOffset = 0;
uint16_t bool_shiftEML = 0;
uint16_t bool_shiftEPC = 0;
uint16_t bool_coilType = 0;
uint16_t bool_testreverse = 0;
uint16_t bool_testeml = 0;
uint16_t bool_testepc = 0;

uint16_t int16_minSpeed = 0;
uint16_t int16_maxSpeed = 0;
uint16_t int16_minRPM = 0;
uint16_t int16_maxRPM = 0;
uint16_t int16_minHall = 0;
uint16_t int16_maxHall = 0;
uint16_t int16_minCAN = 0;
uint16_t int16_maxCAN = 0;
uint16_t int16_shiftRPM = 0;
uint16_t int16_shiftFlashes = 0;

uint16_t int16_tempRPM = 0;
uint16_t int16_speedType = 0;
uint16_t int16_shiftLight = 0;
int label_speedHall = 0;
int label_speedECU = 0;
int label_speedGPS = 0;
int label_speedDSG = 0;
int label_speedDSGUds = 0;
int label_speedHaldexUds = 0;
int label_speedABS = 0;
int label_RPMCAN = 0;
int label_hasCAN = 0;
int label_hasGPS = 0;
int label_paddleUp = 0;

bool autoDiagQuery = false;
double dsgUDSSpeed = 0;
double haldexUDSSpeed = 0;
uint16_t tp20Speed = 0;
uint16_t udsSpeed  = 0;

// SavvyCAN analyzer globals
bool    analyzerMode     = false;
bool    analyzerSerial   = false;
uint8_t analyzerProtocol = 0; // ANALYZER_PROTOCOL_GVRET
uint8_t gpsSatellites = 0;
uint16_t bool_autoDiagQuery = 0;
int label_paddleDown = 0;
int label_reverseActive = 0;
int label_emlActive = 0;
int label_epcActive = 0;

uint16_t graph = 0;
uint16_t mainTime = 0;
volatile bool updates = false;
