#include <TinyGPSPlus.h>     // included for GPS
#include <SoftwareSerial.h>  // included for GPS
#include <ESP32_CAN.h>       // included for CAN
#include <Preferences.h>     // for eeprom/remember settings
#include <ESPUI.h>           // included for WiFi pages
#include <WiFi.h>            // included for WiFi pages
#include <ESPmDNS.h>         // included for WiFi pages
//#include <ButtonLib.h>       // included for paddles
#include "InterruptButton.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncHTTPUpdateServer.h>

/* Defines */
// Debug statements
#define enableDebug 0           // for enabling Serial talkback
#define ChassisCANDebug 0       // for CAN 1 (Chassis) messages
#define detailedDebugStack 0    // for stack sizes (for task memory allocation)
#define detailedDebugWiFi 0     // for wifi feedback
#define detailedDebugEEP 0      // for EEP Serial feedback
#define detailedDebugGPS 0      // for GPS Serial feedback
#define detailedDebugPaddles 0  // for Paddle Serial feedback
#define detailedDebugDSG 0      // for DSG Serial feedback
#define detailedDebugIO 0       // for General IO Serial feedback

#define eepRefresh 2000            // EEPROM save in ms
#define wifiDisable 60000          // turn off WiFi in ms - check for 0 connections after 60s and disable WiFi - burning power otherwise
#define labelRefresh 200           // broadcast refresh rate in ms
#define checkErrorRefresh 500      // for checking if CAN is available (and other errors to be added)
#define serialMonitorRefresh 1000  // Serial Monitor refresh rate in ms
#define durationReset 1500         // for resetting incoming hall if >xms.  To be used to not 'stick' at a speed if signals stop...

// setup - main inputs
#define speedUnits 0                   // 0 = kph, 1 = mph
#define mphFactor 621371               // to convert from kmh > mph
#define wifiHostName "Can2Cluster V2"  // the WiFi name

// setup - tweaky things
#define shiftLightRate 60  // flash EPC at xx ms.  Decreasing may lead to a 'constant' light because of the human eye... ** CAN CHANGE THIS **

// setup - cluster RPM & speed limits
extern uint16_t clusterRPMLimit = 7000;  // rpm limit of the cluster face
extern uint16_t shiftLimit = 6000;       // shift limit (for shift light)
extern uint8_t shiftFlashes = 3;         // number of flashes for shift light
extern uint8_t sweepSpeed = 18;          // for needle sweep rate of change (in ms)
extern uint16_t maxSpeed = 200;          // maximum cluster speed in kmh on the cluster
extern uint16_t maxRPM = 230;            // maximum rpm in hz for the cluster
extern uint16_t maxFreqHall = 200;       // max frequency for top speed using the 02J / 02M hall sensor
extern uint16_t maxFreqRPM = 230;        // max frequency for top speed using the 02J / 02M hall sensor

extern bool useEPCShiftLight = false;  // bool to use the EPC as a shift light
extern bool useEMLShiftLight = false;  // bool to use the EML as a shift light
extern bool useEPCPark = false;        // bool to use the EPC as a shift light
extern bool useEMLPark = false;        // bool to use the EML as a shift light

// setup - step changes (for needle sweep)
extern uint16_t stepRPM = 12;
extern uint16_t stepSpeed = 10;

// tasks (to be able to pause/restart them)
TaskHandle_t handle_processOutputs;    // for enabling/disabling 1000ms frames
TaskHandle_t handle_parseShiftLights;  // for enabling/disabling 1000ms frames

// stack sizes
uint32_t stackshowState = 0;
uint32_t stackcheckError = 0;
uint32_t stackupdateLabels = 0;
uint32_t stackwriteEEP = 0;
uint32_t stackprocessOutputs = 0;
uint32_t stackbroadcastSpeed = 0;
uint32_t stackbroadcastGRA = 0;
uint32_t stackparseGPS = 0;
uint32_t stackparseSpeed = 0;
uint32_t stackparseRPM = 0;
uint32_t stackparseShiftLights = 0;

// setup - pins (output)
#define pinRX_CAN 17   // pin output for SN65HVD230 (CAN_RX)
#define pinTX_CAN 16   // pin output for SN65HVD230 (CAN_TX)
#define pinRX_GPS 14   // pin output for GPS NEO6M (GPS_RX) // mini has these swapped(!)
#define pinTX_GPS 13   // pin output for GPS NEO6M (GPS_TX)
#define pinCoil 18     // pin output for RPM (MK2/High Output Coil Trigger)
#define pinEPC 19      // pin output for EPC
#define pinEML 21      // pin output for EML
#define pinRPM 22      // pin output for RPM
#define pinSpeed 23    // pin output for Speed
#define onboardLED 2   // pin onboard LED
#define pinReverse 26  // pin output for reverse mosfet

// setup - pins (inputs)
#define pinPaddleUp 34    // pin input for DSG paddle up
#define pinPaddleDown 35  // pin input for DSG paddle down
#define pinHallSensor 25  // pin input for Hall Sensor
#define pinMotorInput 39  // pin input for Hall Sensor

// Baud Rates
#define baudSerial 115200  // baud rate for debug
#define baudGPS 9600       // baud rate for the GPS device
#define baudCAN 500000     // baud rate for CAN

// DSG variables
#define PI 3.141592653589793
#define LEVER_P 0x8               // park position
#define LEVER_R 0x7               // reverse position
#define LEVER_N 0x6               // neutral position
#define LEVER_D 0x5               // drive position
#define LEVER_S 0xC               // spot position
#define LEVER_TIPTRONIC_ON 0xE    // tiptronic active
#define LEVER_TIPTRONIC_UP 0xA    // tiptronic up
#define LEVER_TIPTRONIC_DOWN 0xB  // tiptronic down
#define gearPause 20              // Send packets every x ms ** CAN CHANGE THIS **
#define rpmPause 5
#define speedBroadcast 20

// debugging macros
#ifdef enableDebug
#define DEBUG(x, ...) Serial.printf(x "\n", ##__VA_ARGS__)
#define DEBUG_(x, ...) Serial.printf(x, ##__VA_ARGS__)
#else
#define DEBUG(x, ...)
#define DEBUG_(x, ...)
#endif

extern uint8_t vehicleCoolantTemp = 0;  // for vehicle coolant temp
extern uint16_t vehicleRPMCAN = 0;      // current CAN RPM
extern uint16_t vehicleRPMHall = 0;
extern uint16_t vehicleRPM = 0;    // current RPM for cluster
extern uint16_t vehicleSpeed = 0;  // current Speed for cluster
extern uint16_t calcSpeed = 0;     // temp var for calculating speed
extern long tempSpeed = 0;         // for testing only, set fixed speed in kmh.  Can set to 0 to speed up / slow down on repeat with testSpeed enabled
extern long tempRPM = 0;           // for testing only, set fixed speed in kmh.  Can set to 0 to speed up / slow down on repeat with testSpeed enabled

extern double ecuSpeed = 0;   // ECU speed (from analog speed sensor)
extern double dsgSpeed = 0;   // DSG speed (from RPM & Gear), ratios in '_dsg.ino'
extern double gpsSpeed = 0;   // GPS speed (from '_gps.ino')
extern double absSpeed = 0;   // ABS speed (from '_gps.ino')
extern double hallSpeed = 0;  // current Speed.  If no CAN, this will catch dividing by zero by the map function

extern bool useHall = false;
extern bool useECU = false;
extern bool useDSG = false;
extern bool useGPS = false;
extern bool useABS = false;
extern bool coilType = true;

// DSG variables
extern uint8_t gear = 0;   // current gear from DSG
extern uint8_t lever = 0;  // shifter position
extern uint8_t gear_raw = 0;
extern uint8_t lever_raw = 0;
uint32_t lastMillis = 0;         // Counter for sending frames x ms
uint32_t lastMillis2 = 0;        // Counter for sending frames x ms
extern uint32_t lastCAN = 0;     // last CAN message
extern uint32_t lastPaddle = 0;  // last CAN message
extern unsigned long lastPulse = 0;
extern unsigned long lastPulseRPM = 0;
extern unsigned long dutyCycleIncoming = 0;  // Duty Cycle % coming in from Can2Cluster or Hall
extern unsigned long dutyCycleMotor = 0;     // Duty Cycle % coming in from Can2Cluster or Hall

// ECU variables
extern bool vehicleEML = false;          // current EML light status
extern bool vehicleEPC = false;          // current EPC light status
extern bool vehiclePark = false;         // current Park status (from DSG)
extern bool vehicleNeutral = false;      // current Neutral status (from DSG)
extern bool vehicleReverse = false;      // current Reverse status (from DSG)
extern bool vehicleOilPressure = false;  // current oil pressure (from Ford)
extern bool vehicleBattLight = false;    // current battery light (from Ford)
extern uint8_t GRA_counter = 0;
extern uint8_t GRA_crc = 0;

// external variables / triggers
extern bool boolPadUp = false;      // current EML light status
extern bool boolPadUpWiFi = false;  // current EML light status

extern bool boolPadDown = false;      // current EPC light status
extern bool boolPadDownWiFi = false;  // current EPC light status

// for testing / etc
extern bool hasError = false;         // for onboard led flashing
extern bool triggerLED = false;       // to trigger led
extern bool selfTest = false;         // increase RPM/speed slowly, flash lights
extern bool hasNeedleSweep = false;   // do needle sweep on power up?
extern bool hasCAN = false;           // CAN availability check
extern bool hasGPS = false;           // GPS availability check
extern bool tempNeedleSweep = false;  // temporary needle sweep (for testing)
extern bool testSpeedo = false;       // temporary speed check
extern bool testRPM = false;          // temporary rpm check
extern bool tempShiftLight = false;   // temporary shift light check
extern bool testEML = false;          // test engine management light output
extern bool testEPC = false;          // test electronic pedal control light output
extern bool testReverse = false;      // test reverse mosfet output
extern bool testPark = false;         // test park lock output (as EML/EPC)

// define CAN Addresses.  All not req. but here for keepsakes
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588

#define MOTOR_FLEX_ID 0x580
#define GRA_ID 0x38A
#define gear_ID 0x440  // lower 4 bits of byte 2 are gear?

#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x2A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0

#define gearLever_ID 0x448
#define mWaehlhebel_1_ID 0x540  // DQ250 DSG ID

#define HALDEX_ID 0x2C0

#define emeraldECU1_ID 0x1000
#define emeraldECU2_ID 0x1001

#define fordECU1_ID 0x201
#define fordECU2_ID 0x420

// for main functions
extern void basicInit(void);
extern void checkError();
extern void setupTasks();
extern void canInit(void);
extern void onBodyRX(void);
extern void needleSweep(void);
extern void setupPins(void);
extern void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML, bool boolRPM, bool boolSpeed);
extern void broadcastSpeed();
extern void broadcastGRA();
extern void setFrequencySpeed();
extern void setFrequencyRPM();
extern void incomingHz();
extern void incomingMotorSpeed();
extern void parseSpeed();
extern void parseRPM();
extern void parseShiftLights();
extern void processOutputs();

// for EEP
extern void readEEP();
extern void writeEEP();

// for buttons
extern void padUpFunc(void);
extern void padDownFunc(void);

// for WiFi Function Prototypes
extern void connectWifi();
extern void disconnectWifi();
extern void setupUI();
extern void textCallback(Control *sender, int type);
extern void generalCallback(Control *sender, int type);
extern void selectCallback(Control *sender, int type);
extern void updateCallback(Control *sender, int type);
extern void getTimeCallback(Control *sender, int type);
extern void graphAddCallback(Control *sender, int type);
extern void graphClearCallback(Control *sender, int type);
extern void randomString(char *buf, int len);
extern void extendedCallback(Control *sender, int type, void *param);
extern void updateLabels();

// WiFi UI handles
uint16_t bool_NeedleSweep, int16_sweepSpeed, int16_stepSpeed, int16_stepRPM;
uint16_t bool_testSpeedo, bool_testRPM, int16_tempSpeed, bool_useDSG, bool_useGPS, bool_useHall, bool_useABS, bool_testEML, bool_testEPC, bool_testReverse, bool_testPark;

uint16_t bool_positiveOffset, int16_speedOffset, bool_shiftEML, bool_shiftEPC, bool_coilType;
uint16_t int16_maxSpeed, int16_maxHall, int16_maxCAN, int16_shiftRPM, int16_shiftFlashes;
uint16_t int16_maxRPM, int16_tempRPM, int16_clusterRPM, int16_RPMScaling, int16_maxHallRPM;
uint16_t int16_speedType, int16_shiftLight, int16_parkLock;
int label_speedHall, label_speedGPS, label_speedDSG, label_speedABS, label_speedECU, label_RPMCAN, label_RPMHall, label_hasCAN, label_hasGPS, label_paddleUp, label_paddleDown, label_reverseActive, label_emlActive, label_epcActive, label_parkActive;

uint16_t graph;
uint16_t mainTime;
volatile bool updates = false;