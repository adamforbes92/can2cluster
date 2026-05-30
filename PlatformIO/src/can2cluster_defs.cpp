#include "can2cluster_defs.h"

// setup - cluster RPM & speed limits
uint16_t clusterRPMLimit = 7000;
uint16_t shiftLimit = 6000;
uint8_t shiftFlashes = 3;
uint8_t sweepSpeed = 18;
uint16_t maxSpeed = 200;
uint16_t maxRPM = 230;
uint16_t maxFreqHall = 200;
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

bool rpmTrigger = true;
bool speedTrigger = true;

uint8_t gear = 0;
uint8_t lever = 0;
uint8_t gear_raw = 0;
uint8_t lever_raw = 0;
uint32_t lastMillis = 0;
uint32_t lastMillis2 = 0;
uint32_t lastCAN = 0;
volatile unsigned long lastPulse = 0;
volatile unsigned long dutyCycleIncoming = 0;
volatile unsigned long dutyCycleMotor = 0;
volatile unsigned long lastPulseRPM = 0;

bool vehicleEML = false;
bool vehicleEPC = false;
bool vehiclePark = false;
bool vehicleNeutral = false;
bool vehicleReverse = false;
bool vehicleOilPressure = false;
bool vehicleBattLight = false;
uint8_t GRA_counter = 0;
uint8_t GRA_crc = 0;

bool boolPadUp = false;
bool boolPadDown = false;
volatile bool padUpTxPending = false;
volatile bool padDownTxPending = false;

bool useHall = false;
bool useECU = false;
bool useDSG = false;
bool useGPS = false;
bool useABS = false;
bool useTP20 = false;
bool useUDS  = false;
bool useHallRPM = false;
bool coilType = true;

bool hasError = false;
bool triggerLED = false;
bool selfTest = false;
bool hasNeedleSweep = false;
bool hasCAN = false;

bool hasGPS = false;
bool gpsUnavailable = false;
bool gpsError = false;
uint8_t gpsUpdateRateHz = 1;
TaskHandle_t gpsTaskHandle = NULL;
TaskHandle_t updateSpeedHandle = NULL;
TaskHandle_t updateRPMHandle = NULL;
bool tempNeedleSweep = false;
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

uint32_t stackbroadcastGRA = 0;
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
