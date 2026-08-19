#ifndef CAN2CLUSTER_DEFS_H
#define CAN2CLUSTER_DEFS_H

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <TinyGPSPlus.h> // included for GPS
#include <driver/twai.h>  // TWAI (CAN) for ESP32
#include <Preferences.h>    // for eeprom/remember settings
#include <WiFi.h>           // included for WiFi pages
#include <ESPmDNS.h>        // included for WiFi pages
#include <OneButton.h>

#define FW_VERSION "3.20"

#define COOLANT_CAL_MAX 12 // max calibration points for the coolant temp gauge
// LEDC 10-bit resolution at the 80 MHz APB clock tops out at 80e6/1024 ≈ 78125 Hz.
#define COOLANT_PWM_FREQ_MAX 78000

/* Defines */
// Debug statements
#define serialDebug 0        // if 1, will use Serial talkback ** CAN CHANGE THIS **
#define ChassisCANDebug 0    // if 1, will print CAN 2 (Chassis) messages ** CAN CHANGE THIS **
#define serialDebugWifi 0    // for wifi feedback
#define serialDebugEEP 0     // for EEP Serial feedback
#define serialDebugGPS 0     // for GPS Serial feedback
#define serialDebugPaddles 0 // for Paddle Serial feedback
#define serialDebugDSG 0     // for DSG Serial feedback
#define serialDebugIO 0      // for General IO Serial feedback
#define serialDebugCAN 0     // for CAN/TWAI driver Serial feedback
#define detailedDebugStack 0 // for showing stack usage of each task (in showState() task)

#define serialMonitorRefresh 1000 // serial monitor feedback in ms
#define eepRefresh 5000           // EEPROM save in ms
#define labelRefresh 200          // wifi label refresh in ms
#define broadcastSpeedRefresh 20  // speed sending via. CAN in ms
#define broadcastGRARefresh 20    // paddle (GRA) sending via. CAN in ms
#define gearPause 20              // vTaskDelay (in _dsg.ino) for DSG refreshes
#define rpmPause 50               // vTaskDelay for the RPM & Speed output tasks (50 ms = 20 Hz). The output frequency only changes on a real value change, so a faster loop just burns CPU and bogs down the other core-1 tasks.

// global object declarations
extern HardwareSerial ss; // UART2 for GPS (NEO-6M)
extern TinyGPSPlus gps;
extern Preferences pref;
extern OneButton btnPadUp;
extern OneButton btnPadDown;

// setup - main inputs
#define mphFactor 621371              // to convert from kmh > mph (multiply, then /1000000)
#define wifiHostName "Can2Cluster V3" // the WiFi name

// setup - tweaky things
#define shiftLightRate 100 // flash EPC at xx ms.  Decreasing may lead to a 'constant' light because of the human eye... ** CAN CHANGE THIS **
#define durationReset 1500 // ms with no pulse before Hall/RPM input is considered stale and reset to 0

// setup - cluster RPM & speed limits
extern uint16_t clusterRPMLimit; // rpm limit of the cluster face
extern uint16_t shiftLimit;      // shift limit (for shift light)
extern uint8_t shiftFlashes;     // number of flashes for shift light
extern uint8_t sweepSpeed;       // for needle sweep rate of change (in ms)
extern uint16_t maxSpeed;        // maximum cluster speed in kmh on the cluster
extern uint16_t maxRPM;          // maximum rpm in hz for the cluster
extern uint16_t maxFreqHall;     // max frequency for top speed using the 02J / 02M hall sensor
extern bool useEPCShiftLight;    // bool to use the EPC as a shift light
extern bool useEMLShiftLight;    // bool to use the EML as a shift light

// setup - step changes (for needle sweep)
extern uint16_t stepRPM;
extern uint16_t stepSpeed;

// setup - pins (output)
#define pinRX_CAN 17  // pin output for SN65HVD230 (CAN_RX)
#define pinTX_CAN 16  // pin output for SN65HVD230 (CAN_TX)
#define pinRX_GPS 14  // pin output for GPS NEO6M (GPS_RX)
#define pinTX_GPS 13  // pin output for GPS NEO6M (GPS_TX)
#define pinCoil 18    // pin output for RPM (MK2/High Output Coil Trigger)
#define pinEPC 19     // pin output for EPC
#define pinEML 21     // pin output for EML
#define pinRPM 22     // pin output for RPM
#define pinSpeed 23   // pin output for Speed
#define onboardLED 2  // pin onboard LED
#define pinReverse 26 // pin output for reverse mosfet

// setup - pins (inputs)
#define pinPaddleUp 34   // pin input for DSG paddle up
#define pinPaddleDown 35 // pin input for DSG paddle down
#define pinHallSensor 25 // pin input for Hall Sensor
#define pinRpmPulse 39   // pin input for engine RPM pulse

// Baud Rates
#define baudSerial 115200 // baud rate for debug
#define baudGPS 9600      // baud rate for the GPS device
#define baudCAN 500000    // baud rate for CAN

// DSG variables
#ifndef PI
#define PI 3.141592653589793
#endif
#define LEVER_P 0x8              // park position
#define LEVER_R 0x7              // reverse position
#define LEVER_N 0x6              // neutral position
#define LEVER_D 0x5              // drive position
#define LEVER_S 0xC              // spot position
#define LEVER_TIPTRONIC_ON 0xE   // tiptronic active
#define LEVER_TIPTRONIC_UP 0xA   // tiptronic up
#define LEVER_TIPTRONIC_DOWN 0xB // tiptronic down

// General / system debug (master flag). Use #if (not #ifdef): the flags are
// #define'd to 0/1, so #ifdef would always be true and DEBUG would print even
// when serialDebug is 0. Call sites must carry their own [Tag] prefix.
#if serialDebug
#define DEBUG(x, ...) Serial.printf(x "\n", ##__VA_ARGS__)
#define DEBUG_(x, ...) Serial.printf(x, ##__VA_ARGS__)
#else
#define DEBUG(x, ...)
#define DEBUG_(x, ...)
#endif

// Category-specific debug macros
#if serialDebugWifi
#define DEBUG_WIFI(x, ...) Serial.printf("[WiFi] " x "\n", ##__VA_ARGS__)
#define DEBUG_WIFI_(x, ...) Serial.printf("[WiFi] " x, ##__VA_ARGS__)
#else
#define DEBUG_WIFI(x, ...)
#define DEBUG_WIFI_(x, ...)
#endif

#if serialDebugEEP
#define DEBUG_EEP(x, ...) Serial.printf("[EEP] " x "\n", ##__VA_ARGS__)
#define DEBUG_EEP_(x, ...) Serial.printf("[EEP] " x, ##__VA_ARGS__)
#else
#define DEBUG_EEP(x, ...)
#define DEBUG_EEP_(x, ...)
#endif

#if serialDebugGPS
#define DEBUG_GPS(x, ...) Serial.printf("[GPS] " x "\n", ##__VA_ARGS__)
#define DEBUG_GPS_(x, ...) Serial.printf("[GPS] " x, ##__VA_ARGS__)
#else
#define DEBUG_GPS(x, ...)
#define DEBUG_GPS_(x, ...)
#endif

#if serialDebugPaddles
#define DEBUG_PADDLES(x, ...) Serial.printf("[Paddles] " x "\n", ##__VA_ARGS__)
#define DEBUG_PADDLES_(x, ...) Serial.printf("[Paddles] " x, ##__VA_ARGS__)
#else
#define DEBUG_PADDLES(x, ...)
#define DEBUG_PADDLES_(x, ...)
#endif

#if serialDebugDSG
#define DEBUG_DSG(x, ...) Serial.printf("[DSG] " x "\n", ##__VA_ARGS__)
#define DEBUG_DSG_(x, ...) Serial.printf("[DSG] " x, ##__VA_ARGS__)
#else
#define DEBUG_DSG(x, ...)
#define DEBUG_DSG_(x, ...)
#endif

#if serialDebugIO
#define DEBUG_IO(x, ...) Serial.printf("[IO] " x "\n", ##__VA_ARGS__)
#define DEBUG_IO_(x, ...) Serial.printf("[IO] " x, ##__VA_ARGS__)
#else
#define DEBUG_IO(x, ...)
#define DEBUG_IO_(x, ...)
#endif

#if ChassisCANDebug
#define DEBUG_CHASSIS_CAN(x, ...) Serial.printf("[CAN2] " x "\n", ##__VA_ARGS__)
#define DEBUG_CHASSIS_CAN_(x, ...) Serial.printf("[CAN2] " x, ##__VA_ARGS__)
#else
#define DEBUG_CHASSIS_CAN(x, ...)
#define DEBUG_CHASSIS_CAN_(x, ...)
#endif

#if serialDebugCAN
#define DEBUG_CAN(x, ...) Serial.printf("[CAN] " x "\n", ##__VA_ARGS__)
#define DEBUG_CAN_(x, ...) Serial.printf("[CAN] " x, ##__VA_ARGS__)
#else
#define DEBUG_CAN(x, ...)
#define DEBUG_CAN_(x, ...)
#endif

#if detailedDebugStack
#define DEBUG_STACK(x, ...) Serial.printf("[Stack] " x "\n", ##__VA_ARGS__)
#define DEBUG_STACK_(x, ...) Serial.printf("[Stack] " x, ##__VA_ARGS__)
#else
#define DEBUG_STACK(x, ...)
#define DEBUG_STACK_(x, ...)
#endif

// UDS debug macro (always on if any debug is enabled, since UDS is diagnostic)
#if serialDebug
#define DEBUG_UDS(x, ...) Serial.printf("[VW_UDS] " x "\n", ##__VA_ARGS__)
#define DEBUG_UDS_(x, ...) Serial.printf("[VW_UDS] " x, ##__VA_ARGS__)
#else
#define DEBUG_UDS(x, ...)
#define DEBUG_UDS_(x, ...)
#endif

extern uint8_t vehicleCoolantTemp; // for vehicle coolant temp
extern uint16_t vehicleRPMCAN;     // current CAN RPM
extern uint16_t vehicleRPM;        // current RPM for cluster
extern uint16_t vehicleSpeed;      // current Speed for cluster
extern uint16_t calcSpeed;         // temp var for calculating speed
extern long tempSpeed;             // for testing only, set fixed speed in kmh.  Can set to 0 to speed up / slow down on repeat with testSpeed enabled
extern long tempRPM;               // for testing only, set fixed speed in kmh.  Can set to 0 to speed up / slow down on repeat with testSpeed enabled
extern long frequencyRPM;          // inital / base freq.
extern long frequencySpeed;        // inital / base freq.

extern double ecuSpeed;  // ECU speed (from analog speed sensor)
extern double dsgSpeed;  // DSG speed (from RPM & Gear), ratios in '_dsg.ino'
extern double gpsSpeed;  // GPS speed (from '_gps.ino')
extern double absSpeed;  // ABS speed (from '_gps.ino')
extern double hallSpeed; // current Speed.  If no CAN, this will catch dividing by zero by the map function

extern bool rpmTrigger;
extern bool speedTrigger;

// DSG variables
extern uint8_t gear;         // current gear from DSG
extern uint8_t lever;        // shifter position
extern uint8_t gear_raw;     // gear 'raw' data from DSG
extern uint8_t lever_raw;    // lever 'raw' data from DSG
extern uint32_t lastMillis;  // Counter for sending frames x ms
extern uint32_t lastMillis2; // Counter for sending frames x ms
extern uint32_t lastCAN;     // last CAN message
extern volatile unsigned long lastPulse;
extern volatile unsigned long dutyCycleIncoming; // Duty Cycle % coming in from Can2Cluster or Hall
extern volatile unsigned long dutyCycleMotor;    // incoming engine RPM pulse frequency (Hz)
extern volatile unsigned long lastPulseRPM;      // timestamp of last engine RPM pulse

// ECU variables
extern bool vehicleEML;         // current EML light status
extern bool vehicleEPC;         // current EPC light status
extern bool vehiclePark;        // current Park status (from DSG)
extern bool vehicleNeutral;     // current Neutral status (from DSG)
extern bool vehicleReverse;     // current Reverse status (from DSG)
extern bool vehicleOilPressure; // current oil pressure (from Ford)
extern bool vehicleBattLight;   // current battery light (from Ford)
extern uint8_t GRA_counter;     // for paddle frames
extern uint8_t GRA_crc;         // for paddle frames

// external variables / triggers
extern bool boolPadUp;                 // current EML light status
extern bool boolPadDown;               // current EPC light status
extern volatile bool padUpTxPending;   // one-shot CAN transmit trigger for paddle up
extern volatile bool padDownTxPending; // one-shot CAN transmit trigger for paddle down

// for eep - settings via. WiFi
extern bool useHall;    // type of speed input to use: hall sensor
extern bool useECU;     // type of speed input to use: ECU via. CAN (MOTOR2_ID)
extern bool useDSG;     // type of speed input to use: DSG via. CAN (parseDSG) - based on RPM/Current Gear (ratios are key!)
extern bool useGPS;     // type of speed input to use: GPS Module (Neo6M)
extern bool useABS;     // type of speed input to use: ABS via. CAN (BRAKES3_ID)
extern bool useTP20;    // type of speed input to use: TP2.0 DSG speed via. CAN
extern bool useUDS;     // type of speed input to use: UDS speed via. CAN
extern bool useHallRPM; // type of RPM input to use: hall pulse (GPIO39) vs CAN
extern bool coilType;   // has 'old' RPM output
extern bool useMPH;     // display/output cluster speed in MPH instead of km/h

extern bool hasError;       // for flashing onboard LED (no CAN)
extern bool triggerLED;     // bool to flipfloo LED
extern bool diagTest;       // bench diagnostic mode: cycles every output to verify hardware (persisted)
extern bool hasNeedleSweep; // do needle sweep on power up?
extern bool hasCAN;         // bool for 'has CAN' coming in

extern bool hasGPS;         // bool for 'has GPS' >1 satellite
extern bool gpsUnavailable; // true when GPS hardware not responding (timeout)
extern bool gpsError;
extern uint8_t gpsUpdateRateHz; // persisted GPS update rate: 1/5/10/16 Hz
extern TaskHandle_t gpsTaskHandle;
extern TaskHandle_t updateSpeedHandle;
extern TaskHandle_t updateRPMHandle;
extern bool tempNeedleSweep;              // bool to set flag for temp needle sweep (for testing)
extern bool testSpeedo;                   // for testing only, vary final pwmFrequency for speed
extern bool testRPM;                      // for testing only, vary final pwmFrequency for RPM
extern bool useAftermarket;               // speed input: custom CAN (aftermarket)
extern uint32_t aftermarketSpeedID;       // CAN ID to listen on for aftermarket speed
extern uint8_t aftermarketSpeedLowByte;   // byte index holding the speed LSB
extern uint8_t aftermarketSpeedHighByte;  // byte index holding the speed MSB
extern bool aftermarketSpeedLittleEndian; // true = LSB at lowByte index
extern float aftermarketSpeedScale;       // scale applied to raw value
extern int16_t aftermarketSpeedOffset;    // offset applied after scale
extern double aftermarketSpeed;           // parsed speed from aftermarket CAN frame
extern bool broadcastSpeedEnabled;        // bool to enable speed CAN frame broadcast
extern uint32_t broadcastSpeedID;         // CAN ID used for speed broadcast frames
extern uint8_t broadcastSpeedDLC;         // DLC used for speed broadcast frames (0-8)
extern uint8_t broadcastSpeedLowByte;     // payload byte index for speed low byte
extern uint8_t broadcastSpeedHighByte;    // payload byte index for speed high byte
extern bool broadcastSpeedLittleEndian;   // true = low/high order, false = high/low
extern float broadcastSpeedScale;         // scale applied to final vehicleSpeed before send
extern int16_t broadcastSpeedOffset;      // offset applied after scaling before send
extern uint16_t broadcastSpeedValue;      // last computed value packed into speed frame
extern uint8_t broadcastSpeedData[8];     // static payload template bytes
extern bool tempShiftLight;               // for testing only, flash EML/EPC if set
extern bool testEML;                      // bool to force turn on EML
extern bool testEPC;                      // bool to force turn on EPC
extern bool testReverse;                  // bool to force turn on Reverse
extern String dsgParkMode;                // DSG Park behavior: "None", "EML", or "EPC"

// Coolant temperature gauge (PWM on a shared ULN2003 output, EML or EPC pin)
extern uint8_t coolantOutput;                  // 0=Off, 1=EML pin, 2=EPC pin (mutually exclusive with that pin's light features)
extern uint32_t coolantPwmFreq;                // fixed PWM carrier frequency (Hz) driving the gauge
extern uint8_t coolantWarnTemp;                // idiot-light threshold (deg C): peg gauge & warning lamp at/above this
extern uint8_t coolantCalCount;                // number of calibration points in use
extern int16_t coolantCalTemp[COOLANT_CAL_MAX];  // calibration temperature points (deg C), kept sorted ascending
extern uint16_t coolantCalDuty[COOLANT_CAL_MAX]; // calibration duty points (0-1023), paired with coolantCalTemp
extern bool coolantCalMode;                    // when true, output is driven at coolantCalDutyNow so the needle can be read
extern uint16_t coolantCalDutyNow;             // live jog duty used while calibrating (0-1023)
extern uint16_t coolantAppliedDuty;            // last duty actually written to the gauge (for status/curve)

// Blink LED state
struct BlinkState
{
  bool active;
  int flashCount;
  int currentFlash;
  int duration;
  unsigned long lastToggleTime;
  bool outputState;
  bool boolEPC;
  bool boolEML;
  bool boolRPM;
  bool boolSpeed;
};
extern BlinkState blinkState;

// for stack monitoring (tasks!)
extern uint32_t stackShowState;
extern uint32_t stackUpdateLabels;
extern uint32_t stackWriteEEP;

extern uint32_t stackbroadcastGRA;
extern uint32_t stackbroadcastSpeed;
extern uint32_t stackparseGPS;
extern uint32_t stackparseDSG;

extern uint32_t stackupdateSpeed;
extern uint32_t stackupdateRPM;
extern uint32_t stackshiftLight;
extern uint32_t stackcheckError;

// define CAN Addresses.  All not req. but here for keepsakes
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588

#define MOTOR_FLEX_ID 0x580
#define GRA_ID 0x38A
#define gear_ID 0x440 // lower 4 bits of byte 2 are gear?

#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x2A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0

#define gearLever_ID 0x448
#define mWaehlhebel_1_ID 0x540 // DQ250 DSG ID

#define HALDEX_ID 0x2C0

#define emeraldECU1_ID 0x1000
#define emeraldECU2_ID 0x1001

#define fordECU1_ID 0x201
#define fordECU2_ID 0x420

// MQB platform CAN addresses (from the OpenHaldex project / vw_mqb.dbc + MQB FCAN K-matrix).
// All not req. but here for keepsakes
#define LWI_01 0x086        // steering-angle sensor (Lenkwinkelinformation)
#define ESP_14 0x08A        // ESP-to-AWD coupling-range limits
#define MOTOR_11 0x0A7      // engine torque demand/output broadcast
#define MOTOR_12 0x0A8      // high-rate engine speed/torque broadcast (RPM)
#define GETRIEBE_11 0x0AD   // transmission/DSG status broadcast (gear lever, target gear)
#define GETRIEBE_17 0x0B1   // DSG paddle/tip status
#define ESP_19 0x0B2        // per-wheel ABS speeds broadcast
#define ESP_21 0x0FD        // ESP/ASR mode + vehicle reference speed
#define ESP_02 0x101        // ESP broadcast
#define EPB_01 0x104        // electronic parking brake state
#define ESP_05 0x106        // brake pressure + brake-light/brake-pedal flags
#define MOTOR_04 0x107      // engine torque/charge broadcast (boost)
#define ESP_10 0x116        // lateral dynamics / yaw + lateral accel
#define MOTOR_20 0x121      // accelerator pedal raw/filtered + status
#define ESP_18 0x135        // ESP minor broadcast
#define ESP_29 0x18C        // ESP broadcast
#define KOMBI_01 0x30B      // instrument cluster broadcast (handbrake)
#define BLINKMODI_02 0x366  // hazard / turn-signal status broadcast
#define CHARISMA_01 0x385   // drive-profile (Charisma) selection
#define ESP_07 0x392        // ESP broadcast
#define MOTOR_14 0x3BE      // low-rate engine state/event broadcast (kickdown)
#define GETRIEBE_14 0x3C8   // transmission broadcast
#define GATEWAY_72 0x3DB    // gateway body lighting state
#define PARKHILFE_04 0x54B  // park-assist broadcast
#define SYSTEMINFO_01 0x585 // system info broadcast
#define ESP_23 0x5BE        // ESP broadcast
#define MOTOR_07 0x640      // low-rate engine data broadcast (coolant temp)
#define MOTOR_CODE_01 0x641 // engine code broadcast
#define ESP_20 0x65D        // ESP broadcast
#define MOTOR_18 0x670      // low-rate engine status broadcast (EPC lamp)
#define DIAGNOSE_01 0x6B2   // diagnostics broadcast
#define KOMBI_02 0x6B7      // instrument cluster broadcast

// MQB Getriebe_11 GE_Fahrstufe (gear-lever position) values — byte 5 bits 2..5.
// Verified against OpenHaldex MQB log "gears all inc tip and sport.csv".
#define MQB_FAHRSTUFE_INIT 0x0 // init / no display
#define MQB_FAHRSTUFE_P 0x5    // park
#define MQB_FAHRSTUFE_R 0x6    // reverse
#define MQB_FAHRSTUFE_N 0x7    // neutral
#define MQB_FAHRSTUFE_D 0x8    // drive
#define MQB_FAHRSTUFE_S 0x9    // sport
#define MQB_FAHRSTUFE_TIP 0xD  // tiptronic / manual gate

// for main functions
extern void basicInit(void);
extern void checkError(void *args);
extern void canInit(void);
extern void onBodyRX(const twai_message_t &frame);
extern void needleSweep(void);
extern void setupPins(void);
extern void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML, bool boolRPM, bool boolSpeed);
extern void updateBlinkLED(void);
extern void broadcastSpeed(void *args);
extern void broadcastGRA(void *args);
extern void setFrequencySpeed(long frequencyHz);
extern void setFrequencyRPM(long frequencyHz);
extern void incomingHz();
extern void incomingRPMHz();

// for EEP
extern void readEEP();
extern void writeEEP();

// for tasks
extern void setupTasks();
extern void showState();
extern void diagTestTask(void *args);

// for buttons
extern void padUpFunc(void);
extern void padDownFunc(void);
extern void updateButtons(void);
extern void updatePaddleFeedback(void);

// for WiFi Function Prototypes
extern void connectWifi();
extern void disconnectWifi();
extern void setupUI();

extern bool autoDiagQuery;
extern double dsgUDSSpeed; // legacy alias (TP2.0 DSG speed)
extern double haldexUDSSpeed;
extern uint16_t tp20Speed; // speed from TP2.0 protocol
extern uint16_t udsSpeed;  // speed from UDS protocol

// SavvyCAN / Analyzer
extern bool analyzerMode;        // WiFi GVRET/SLCAN (TCP port 23)
extern bool analyzerSerial;      // Serial GVRET (SavvyCAN over USB, 1 Mbaud)
extern uint8_t analyzerProtocol; // ANALYZER_PROTOCOL_GVRET or ANALYZER_PROTOCOL_LAWICEL
extern uint8_t gpsSatellites;

extern int label_speedDSGUds;
extern int label_speedHaldexUds;
extern uint16_t bool_autoDiagQuery;

#endif // CAN2CLUSTER_DEFS_H