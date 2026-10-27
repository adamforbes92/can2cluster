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

// V4.00 - two boards from one codebase: ESP32 DevKit + ESP32-S3 board.
//         S3 gets the dedicated coolant gauge output (MCP4725 current sink).
// V4.01 - MQB paddles rebuilt from the Tplus/Tminus log review (see MQB_PADDLES.md).
// V4.02 - MK4 cruise stalk + PQ gateway frames, faster needle sweep, settable speedo dial top.
// V4.04 - fuel pump relay on the EML or EPC output (prime at power-up, then on while CAN RPM is there).
#define FW_VERSION "4.04"

// ===== Board =====
// Which PCB this firmware is built for. Do NOT change it here - pick the env in
// platformio.ini instead ("esp32doit-devkit-v1" or "esp32-s3"), which sets
// -DBOARD_DEVKIT=1 or -DBOARD_S3=1.
#ifndef BOARD_S3
#define BOARD_S3 0 // 0 = ESP32 DevKit board, 1 = ESP32-S3 board
#endif

#if BOARD_S3
#define BOARD_ID "esp32s3"           // board id used by OTA / releases.json (must match make_release.py)
#define BOARD_NAME "ESP32-S3 board"  // shown on the web page
#else
#define BOARD_ID "esp32"             // board id used by OTA / releases.json (must match make_release.py)
#define BOARD_NAME "ESP32 DevKit board" // shown on the web page
#endif

#define COOLANT_CAL_MAX 12 // max calibration points for the coolant temp gauge
// Coolant gauge PWM (DevKit only - the only LEDC output left; speed/RPM are
// GPTimer square waves). LEDC 10-bit at the 80 MHz APB clock tops out at
// 80e6/1024 ≈ 78125 Hz.
#define COOLANT_PWM_FREQ_MAX 78000

// Where the coolant gauge is driven from (the value saved as coolantOutput)
#define COOLANT_OFF 0       // coolant gauge not used
#define COOLANT_ON_EML 1    // DevKit: PWM on the EML output (EML light can't be used)
#define COOLANT_ON_EPC 2    // DevKit: PWM on the EPC output (EPC light can't be used)
#define COOLANT_ON_GAUGE 3  // S3: dedicated COOLANT_OUT pin (J1 pin 16), current sink from the MCP4725

// ===== Fuel pump relay =====
// The EML or EPC output can switch a fuel pump relay coil (the output pulls the
// coil's earth side down). At power-up the pump primes for fuelPumpPrimeSeconds,
// then stays on for as long as the ECU is sending RPM on CAN.
#define FUEL_PUMP_PRIME_DEFAULT 3     // seconds the pump primes for at power-up (like the factory 2-3 s prime)
#define FUEL_PUMP_PRIME_MAX 30        // longest prime the web page allows, seconds
#define FUEL_PUMP_MIN_RPM 50          // CAN RPM at or above this = engine cranking or running (cranking reads ~150-300)
#define FUEL_PUMP_RPM_TIMEOUT_MS 1000 // no RPM frame for this long = treat RPM as gone and stop the pump
#define FUEL_PUMP_OFF 0               // web page: pump relay off
#define FUEL_PUMP_PRIMING 1           // web page: pump on, power-up prime
#define FUEL_PUMP_RUNNING 2           // web page: pump on, engine RPM seen on CAN

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
#define serialDebugI2C 0     // for I2C (S3 board MCP4725 coolant DAC) Serial feedback
#define detailedDebugStack 0 // for showing stack usage of each task (in showState() task)

#define serialMonitorRefresh 1000 // serial monitor feedback in ms
#define eepRefresh 5000           // EEPROM save in ms
#define labelRefresh 200          // wifi label refresh in ms
#define broadcastSpeedRefresh 20  // speed sending via. CAN in ms
#define broadcastPaddlesRefresh 10 // paddle task period in ms: reads the paddles every 10 ms; PQ GRA goes out every 2nd loop (20 ms), MQB 0x3DD every 10th (100 ms) or straight away on a change
#define gearPause 20              // vTaskDelay (in _dsg.ino) for DSG refreshes
#define rpmPause 50               // vTaskDelay for the RPM & Speed output tasks (50 ms = 20 Hz). The output frequency only changes on a real value change, so a faster loop just burns CPU and bogs down the other core-1 tasks.

// DSG speed smoothing: gear and RPM arrive on separate CAN frames, so during a
// shift they are momentarily inconsistent and the calculated speed spikes/bucks.
#define dsgGearSettleMs 300       // hold last DSG speed for this long after a gear change (real road speed barely moves during a shift)
#define dsgSpeedSmoothing 0.35    // low-pass factor 0..1 (1 = no smoothing) applied to DSG speed to remove residual RPM jitter

// global object declarations
extern HardwareSerial ss; // UART2 for GPS (NEO-6M)
extern TinyGPSPlus gps;
extern Preferences pref;
extern OneButton btnPadUp;
extern OneButton btnPadDown;

// setup - main inputs
#define mphFactor 621371              // to convert from kmh > mph (multiply, then /1000000)
#define wifiHostName "Can2Cluster" // the WiFi name

// setup - tweaky things
#define shiftLightRate 100 // flash EPC at xx ms.  Decreasing may lead to a 'constant' light because of the human eye... ** CAN CHANGE THIS **
#define durationReset 1500 // ms with no pulse before Hall/RPM input is considered stale and reset to 0

// setup - cluster RPM & speed limits
extern uint16_t clusterRPMLimit; // rpm limit of the cluster face
extern uint16_t shiftLimit;      // shift limit (for shift light)
extern uint8_t shiftFlashes;     // number of flashes for shift light
extern uint8_t sweepSpeed;       // for needle sweep rate of change (in ms)
extern uint16_t maxSpeed;        // top of the speedo dial (km/h, or mph on an MPH cluster) - where the needle sweep goes to. Set on the web page as "Speedo Dial Top"
#define SENSOR_TOP_SPEED 200     // km/h that the Max Hall / VR Sensor Frequency sliders are set against (was maxSpeed up to V4.01, which was always 200)
#define SPEEDO_DIAL_TOP_MIN 60   // lowest "Speedo Dial Top" the web page accepts
#define SPEEDO_DIAL_TOP_MAX 400  // highest "Speedo Dial Top" the web page accepts
extern uint16_t maxRPM;          // maximum rpm in hz for the cluster
extern uint16_t maxFreqHall;     // max frequency for top speed using the 02J / 02M hall sensor
extern uint16_t maxFreqVR;       // max frequency for top speed using a variable-reluctance (VR) sensor
extern bool useEPCShiftLight;    // bool to use the EPC as a shift light
extern bool useEMLShiftLight;    // bool to use the EML as a shift light

// setup - step changes (for needle sweep)
extern uint16_t stepRPM;
extern uint16_t stepSpeed;

#if BOARD_S3
// ===== Pins - ESP32-S3 board =====
// Taken from PCB/ProPrj_can2clusterV4-s3-reva_2026-10-03 (module U14,
// ESP32-S3-WROOM-1-N16R8). Module pin number in brackets.
// Every output goes through the TPL7407 driver (U12): ESP pin HIGH = output pulled to ground.

// setup - pins (output)
#define pinRX_CAN 14         // CAN0_RX from TCAN1044 RXD (module pin 22)
#define pinTX_CAN 13         // CAN0_TX to TCAN1044 TXD (module pin 21)
#define pinCAN_Standby 12    // CAN0_STANDBY to TCAN1044 STB: LOW = normal, HIGH = standby (module pin 20)
#define pinCAN_Term 1        // CAN_TERM_EN: LOW = 120R termination on, input = off (module pin 39)
#define pinRX_GPS 17         // GPS_TX net = GPS module's TX -> ESP RX (GPS header pin 3, module pin 10)
#define pinTX_GPS 18         // GPS_RX net = ESP TX -> GPS module's RX (GPS header pin 2, module pin 11)
#define pinCoil 11           // ESP_MK2RPM: MK2/high output coil trigger (module pin 19)
#define pinEPC 39            // ESP_EPC: EPC light output (module pin 32)
#define pinEML 40            // ESP_EML: EML light output (module pin 33)
#define pinRPM 10            // ESP_RPM: RPM square wave output (module pin 18)
#define pinSpeed 9           // ESP_SPEED: speed square wave output (module pin 17)
#define pinReverse 38        // ESP_REV: reverse light MOSFET (module pin 31)
#define pinOutputPullUp 41   // ESP_PU12: HIGH = 12V pull-ups on Speed/RPM/EML/EPC outputs (Q4) (module pin 34)
#define pinSpeedPullUp 47    // ESP_SPD_PU: HIGH = 12V pull-up on the hall speed input (Q6/Q5) (module pin 24)
#define pinSpeedPullDown 48  // ESP_SPD_PD: HIGH = 4k7 pull-down on the hall speed input (U12 ch1) (module pin 25)
#define hasOnboardLED 0      // no LED on the S3 board

// setup - pins (inputs)
#define pinPaddleUp 42       // ESP_UP_PAD: DSG paddle up, LOW = pressed (U10 buffer, module pin 35). Cruise main switch in cruise stalk mode
#define pinPaddleDown 21     // ESP_DOWN_PAD: DSG paddle down, LOW = pressed (U10 buffer, module pin 23). Cruise SET in cruise stalk mode
#define pinHallSensor 16     // ESP_SPEED_IN: hall speed sensor (U9 buffer, module pin 9). Cruise RESUME in cruise stalk mode
#define pinVR 8              // VR_COUT: VR conditioner (MAX9924) output, 3V3 pull-up R6 (module pin 12)
#define pinRpmPulse 15       // ESP_RPM_IN: engine RPM pulse (U9 buffer, module pin 8)
#define pinBoardVersion 4    // BOARD_VER: analogue board-ID divider (R34 to ground) (module pin 4)

// setup - I2C bus (only the coolant DAC lives on it)
#define pinI2C_SDA 5         // ESP_SDA, 10k pull-up R22 (module pin 5)
#define pinI2C_SCL 6         // ESP_SCL, 10k pull-up R23 (module pin 6)
#define I2C_FREQ_HZ 400000UL // I2C clock speed (MCP4725 fast mode)
#define MCP4725_ADDR 0x60    // coolant gauge DAC (U5, MCP4725A0, A0 grounded)
#define MCP4725_MAX 4095     // 12-bit DAC full scale = full current through the gauge

// S3 board switches. Fixed at boot for now - set them here.
#define useCANTermination 0  // 1 = switch the 120R CAN termination on (bench / end-of-bus only) ** CAN CHANGE THIS **
#define useOutputPullUp 1    // 1 = 12V pull-ups on Speed/RPM/EML/EPC (needed for clusters with no pull-up of their own) ** CAN CHANGE THIS **
#define speedInputBias 1     // hall speed input: 0 = none, 1 = 12V pull-up (open-collector hall), 2 = pull-down ** CAN CHANGE THIS **

#else
// ===== Pins - ESP32 DevKit board =====
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
#define hasOnboardLED 1 // the DevKit has a blue LED on GPIO2
#define pinReverse 26 // pin output for reverse mosfet

// setup - pins (inputs)
#define pinPaddleUp 34   // pin input for DSG paddle up (connector pin 10). Cruise main switch in cruise stalk mode
#define pinPaddleDown 35 // pin input for DSG paddle down (connector pin 11). Cruise SET in cruise stalk mode
#define pinHallSensor 25 // pin input for Hall Sensor (connector pin 12). Cruise RESUME in cruise stalk mode
#define pinVR 27         // pin input for variable-reluctance (VR) speed sensor (conditioner digital output, ext pull-up to 3.3V)
#define pinRpmPulse 39   // pin input for engine RPM pulse
#endif

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

#if serialDebugI2C
#define DEBUG_I2C(x, ...) Serial.printf("[I2C] " x "\n", ##__VA_ARGS__)
#define DEBUG_I2C_(x, ...) Serial.printf("[I2C] " x, ##__VA_ARGS__)
#else
#define DEBUG_I2C(x, ...)
#define DEBUG_I2C_(x, ...)
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
extern volatile uint32_t lastRPMCANTime; // millis() when an RPM frame last arrived on CAN, 0 = never
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
extern double vrSpeed;   // current Speed from the variable-reluctance (VR) sensor

extern bool rpmTrigger;
extern bool speedTrigger;

// DSG variables
extern uint8_t gear;         // current gear from DSG
extern uint8_t lever;        // shifter position
extern uint8_t gear_raw;     // gear 'raw' data from DSG
extern uint8_t lever_raw;    // lever 'raw' data from DSG
extern float dsgGearRatio[7];  // adjustable gear ratios, index 1..6 (DQ250 defaults)
extern float dsgFinalDrive14;  // final drive for gears 1-4
extern float dsgFinalDrive56;  // final drive for gears 5-6
extern float dsgTireCirc;      // tire circumference in metres
extern uint32_t lastMillis;  // Counter for sending frames x ms
extern uint32_t lastMillis2; // Counter for sending frames x ms
extern uint32_t lastCAN;     // last CAN message
extern volatile unsigned long lastPulse;
extern volatile unsigned long dutyCycleIncoming; // Duty Cycle % coming in from Can2Cluster or Hall
extern volatile unsigned long lastPulseVR;       // timestamp of last VR pulse
extern volatile unsigned long dutyCycleIncomingVR; // incoming VR pulse frequency (Hz)
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
extern uint8_t GRA_counter;     // PQ GRA_Neu (0x38A) message counter, 0-15
extern uint8_t GRA_crc;         // PQ GRA_Neu (0x38A) checksum of the last frame sent

// DSG paddles (see the "DSG paddles" section further down and MQB_PADDLES.md)
extern bool boolPadUp;                          // web page feedback: paddle + pulled in the last 2 s
extern bool boolPadDown;                        // web page feedback: paddle - pulled in the last 2 s
extern volatile bool isPaddleUpHeld;            // paddle + is being held right now (after the debounce)
extern volatile bool isPaddleDownHeld;          // paddle - is being held right now (after the debounce)
extern volatile uint8_t paddleTestCmd;          // web page "Test Paddle" button: MQB_PADDLE_UP/DOWN until the paddle task takes it
extern volatile uint8_t paddleCmdSent;          // paddle bits going out right now (MQB_PADDLE_x), for the web page
extern volatile uint32_t mqbPaddleOtherSenderMs; // millis() when a 0x3DD from another module (a real gateway) last arrived, 0 = never
extern volatile uint32_t graPaddleOtherSenderMs; // millis() when a 0x38A from another module (a real gateway) last arrived, 0 = never

// MQB gearbox read-back, only used to show on the web page (all 0xFF until the frame is seen)
extern volatile uint8_t mqbLeverPosition;  // selector lever position from 0x0AF (MQB_LEVER_x)
extern volatile uint8_t mqbTipState;       // gearbox tip mode from 0x0B1 (MQB_TIP_x)
extern volatile uint8_t mqbGearboxWarning; // gearbox warning for the cluster from 0x394 (0 = none)

// for eep - settings via. WiFi
extern bool useHall;    // type of speed input to use: hall sensor
extern bool useVR;      // type of speed input to use: variable-reluctance (VR) sensor
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
extern bool tempNeedleSweep;              // web page "Test Needle Sweep" button pressed - loop() starts a sweep
extern volatile bool isNeedleSweeping;    // true while the needle sweep is moving the needles (the speed / RPM tasks leave the outputs alone)
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
extern String fuelPumpOutput;             // fuel pump relay output: "None", "EML" or "EPC"
extern uint8_t fuelPumpPrimeSeconds;      // how long the pump primes for at power-up, seconds
extern uint8_t fuelPumpState;             // FUEL_PUMP_OFF / FUEL_PUMP_PRIMING / FUEL_PUMP_RUNNING (shown on the web page)
extern String paddleOutput;               // which paddle frame(s) to send: "Off", "MQB", "PQ", "All" (see PADDLE_OUTPUT_DEFAULT)

// MK4 cruise stalk (see the "MK4 cruise stalk" section further down)
extern bool useCruiseStalk;                    // true = pins 10/11/12 are the MK4 cruise stalk (main / SET / RESUME), not paddles + hall speed
extern volatile bool isCruiseMainOn;           // stalk main switch is on (pin 10 grounded)
extern volatile bool isCruiseSetPressed;       // SET button pressed (pin 11 grounded)
extern volatile bool isCruiseResumePressed;    // RESUME pressed (pin 12 grounded, after the debounce)
extern volatile uint8_t cruiseButtonsByte;     // GRA byte 1 to send: main switch / cancel bits (also shown on the web page)
extern volatile uint8_t cruiseSetResumeBits;   // GRA byte 2 SET / RESUME bits to send (also shown on the web page)

// PQ gateway frames (see the "PQ gateway frames" section further down)
extern bool useGatewayFrames;                  // true = send 0x570 / 0x390 / 0x7D0 like a PQ gateway would
extern volatile uint32_t bsgLastOtherSenderMs;   // millis() when another module last sent 0x570, 0 = never
extern volatile uint32_t gateKomf1OtherSenderMs; // millis() when another module last sent 0x390, 0 = never
extern volatile uint32_t diagnose1OtherSenderMs; // millis() when another module last sent 0x7D0, 0 = never
extern volatile uint32_t clusterKm;              // mileage read from the cluster (Kombi_3 0x520), km
extern volatile uint32_t clusterKmSeenMs;        // millis() when that mileage last arrived, 0 = never

// Coolant temperature gauge
// DevKit: PWM on a shared output (EML or EPC pin). S3: dedicated COOLANT_OUT current sink.
extern uint8_t coolantOutput;                  // COOLANT_OFF / COOLANT_ON_EML / COOLANT_ON_EPC (DevKit) / COOLANT_ON_GAUGE (S3)
extern uint32_t coolantPwmFreq;                // DevKit only: fixed PWM carrier frequency (Hz) driving the gauge
extern uint8_t coolantWarnTemp;                // warning threshold (deg C): peg gauge & warning lamp at/above this
extern uint8_t coolantCalCount;                // number of calibration points in use
extern int16_t coolantCalTemp[COOLANT_CAL_MAX];  // calibration temperature points (deg C), kept sorted ascending
extern uint16_t coolantCalDuty[COOLANT_CAL_MAX]; // calibration output points (0-1023 DevKit PWM / 0-4095 S3 DAC), paired with coolantCalTemp
extern bool coolantCalMode;                    // when true, output is driven at coolantCalDutyNow so the needle can be read
extern uint16_t coolantCalDutyNow;             // live jog value used while calibrating (0-1023 DevKit / 0-4095 S3)
extern uint16_t coolantAppliedDuty;            // last value actually written to the gauge (for status/curve)

// S3 board state
extern bool i2cMcpPresent;            // S3: MCP4725 coolant DAC answered on the I2C bus
extern uint16_t boardVersionMilliVolts; // S3: voltage on the BOARD_VER pin at boot (board-ID divider)

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

extern uint32_t stackbroadcastPaddles;
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

// Renault Megane engine ECU broadcast (confirmed from Megane CAN logs).
// RPM lives in bytes 0..1 (big-endian 16-bit), scaling raw/8, e.g. 4000 RPM = 0x7D00.
// Same value is mirrored on 0x1F9 bytes 2..3.
#define RENAULT_MEGANE_ECU 0x181

// MQB platform CAN addresses (from the OpenHaldex project / vw_mqb.dbc + MQB FCAN K-matrix).
// All not req. but here for keepsakes
#define LWI_01 0x086        // steering-angle sensor (Lenkwinkelinformation)
#define ESP_14 0x08A        // ESP-to-AWD coupling-range limits
#define MOTOR_11 0x0A7      // engine torque demand/output broadcast
#define MOTOR_12 0x0A8      // high-rate engine speed/torque broadcast (RPM)
#define GETRIEBE_11 0x0AD   // transmission/DSG status broadcast (gear lever, target gear)
#define GETRIEBE_12 0x0AE   // gearbox -> engine: rev-match/torque requests during a shift. NOT the paddles (see MQB_PADDLES.md)
#define GETRIEBE_17 0x0B1   // gearbox tip mode (GE_TippZustand): none / M gate / temporary from a paddle
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
#define WBA_03 0x394        // gearbox -> cluster: gear display, M/D, gearbox warnings
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

// ===== DSG paddles =====
// Worked out from the Tplus/Tminus logs and the MQB K-matrix - the full story
// is in MQB_PADDLES.md next to platformio.ini.
//
// MQB: the gearbox reads the steering-wheel paddles from Gateway_74 (0x3DD),
// which the gateway normally sends on the drivetrain CAN. A swap has no
// gateway, so Can2Cluster sends it instead. Byte 7 (K-matrix MQB_ACAN_KMatrix):
//   bit 0 = MFL_Tip_Down        (1 = paddle - pulled)
//   bit 1 = MFL_Tip_Up          (1 = paddle + pulled)
//   bit 2 = LS_Tiptronic_Fehler (paddle fault - always 0)
// No counter or checksum. Sent every 100 ms, plus straight away when a paddle
// changes, and the bit stays set for as long as the paddle is held - the same
// as a factory gateway (customer log "Renault Megane - Paddle Shifters Filter
// Log.csv": shortest taps 2 frames / 60-150 ms, longest 52 frames / 5 s).
#define MQB_PADDLE_ID 0x3DD              // Gateway_74: steering-wheel paddles -> gearbox
#define MQB_PADDLE_NONE 0x00             // byte 7: no paddle pulled
#define MQB_PADDLE_DOWN 0x01             // byte 7 bit 0: MFL_Tip_Down
#define MQB_PADDLE_UP 0x02               // byte 7 bit 1: MFL_Tip_Up
#define MQB_PADDLE_BOTH 0x03             // both pulled together (a factory gateway sends this too)
#define MQB_PADDLE_REFRESH_MS 100        // factory cycle time (K-matrix GenMsgCycleTime)
#define MQB_PADDLE_MIN_HOLD_MS 150       // shortest pull we send: always 2+ frames, like the shortest factory taps
#define MQB_PADDLE_OTHER_SENDER_MS 1000  // a 0x3DD from another module in this long = a real gateway is fitted, so we stay quiet

// PQ: the paddles ride in the cruise-control frame GRA_Neu (GRA_ID 0x38A),
// 4 bytes. Byte map from the VW PQ35/46 K-matrix (V5.20.6F); opendbc agrees:
//   byte 0         GRA_Checksum: byte 1 XOR byte 2 XOR byte 3
//   byte 1         cruise buttons (main switch, cancel, up/down short/long) - all 0
//   byte 2 bit 0   GRA_Neu_Setzen: cruise SET - always 0
//   byte 2 bit 1   GRA_Recall: cruise RESUME - always 0
//   byte 2 bit 2-3 GRA_Sender: which part sends the frame (GRA_SENDER below)
//   byte 2 bit 4-7 GRA_Neu_Zaehler: message counter 0-15, +1 every frame
//   byte 3 bit 0   GRA_Tip_Down: paddle -
//   byte 3 bit 1   GRA_Tip_Up: paddle +
// The K-matrix sends it every 20-30 ms and calls it missing after 500 ms. We send every 20 ms.
// (Up to V4.01 byte 2 was a 0-255 counter: the real counter only moved every
// 16th frame, and cruise SET/RESUME read as pressed in about half the frames.)
#define GRA_PULSE_MIN_MS 80              // (V4.03: no longer used - the PQ tip now uses the shared one-shot pulse, MQB_PADDLE_MIN_HOLD_MS)
#define GRA_COUNTER_MAX 15               // byte 2 counter runs 0-15, then back to 0
#define GRA_SENDER 0                     // ** CAN CHANGE THIS ** GRA_Sender code: 0 = body module, 1 = column stalk 6 buttons, 2 = MQB-style wheel (MFL Gen 2), 3 = column stalk 4 buttons
#define GRA_OTHER_SENDER_MS 1000         // a 0x38A from another module in this long = a real gateway is sending it, so we stay quiet

// Paddle switches and the web page test buttons
#define PADDLE_DEBOUNCE_MS 20            // a paddle switch must read the same for this long to count
#define PADDLE_TEST_HOLD_MS 200          // web page "Test Paddle" button = a pull this long

// V4.03 tip model (from the DSG firmware RE - see Documents/dsg_re/DSG_RE_SUMMARY.md):
// each pull sends ONE clean edge, not a held level. On the paddle's rising edge
// we send the tip bit for one pulse (MQB_PADDLE_MIN_HOLD_MS long) on the chosen
// frame(s), then force the bit back to 0. A new pull is only started by a fresh
// rising edge, after the paddle has been let go AND this lockout has passed.
// The gearbox accepts a tip exactly this way: edge-triggered, a one-shot latch
// that only re-arms on release, and a tap landing in the shift-busy window is
// dropped. The OEM gateway log (229 pulls) shows ~302 ms minimum between shifts.
// The old "hold the level" logic let a release bounce make a second edge (two-
// gear jump) and let a quick re-tap land in the busy window (press-twice).
#define PADDLE_SHIFT_LOCKOUT_MS 300      // smallest gap before the next tip edge is accepted (OEM min ~302 ms)

// Which paddle frame(s) go on the bus (web page drop-down, saved as "paddleMode"):
//   "Off" - nothing sent (non-VW cars, or no paddles fitted)
//   "MQB" - Gateway_74 0x3DD
//   "PQ"  - GRA_Neu 0x38A
//   "All" - both (neither ID is used on the other platform's drivetrain CAN)
#define PADDLE_OUTPUT_DEFAULT "Off"

// ===== MK4 cruise stalk =====
// Added in V4.02, rebuilt from a customer's own MK4 build (v4.01.2) for a PQ
// engine in a car with the MK4 cruise stalk. "Input Mode" on the web page picks
// what connector pins 10, 11 and 12 are used for:
//   DSG Paddles     - pin 10 paddle +, pin 11 paddle -, pin 12 hall speed (as before)
//   MK4 Cruise Stalk - pin 10 main switch, pin 11 SET, pin 12 RESUME, each on when grounded
// In cruise stalk mode the stalk goes to the ECU in GRA_Neu (0x38A, every 20 ms)
// instead of the paddles, the paddle bits are always 0 (the stalk can never shift
// the gearbox) and no 0x3DD is sent. Hall speed isn't available (pin 12 is RESUME).
// Saved as "cruiseStalk" - the same key as the customer's build, so their setting carries across.
// GRA_Neu byte 1 (cruise buttons):
#define GRA_MAIN_SWITCH 0x01      // byte 1 bit 0: GRA_Hauptschalter, main switch on
#define GRA_CANCEL 0x02           // byte 1 bit 1: GRA_Abbrechen, cancel
// GRA_Neu byte 2 (bits 0-1, under the counter and sender code):
#define GRA_SET 0x01              // byte 2 bit 0: GRA_Neu_Setzen, SET
#define GRA_RESUME 0x02           // byte 2 bit 1: GRA_Recall, RESUME
#define CRUISE_CANCEL_MS 200      // switching the main switch off sends "cancel" for this long
#define CRUISE_RESUME_DEBOUNCE_MS 30 // RESUME (pin 12) must read the same for this long to count. Pins 10/11 use PADDLE_DEBOUNCE_MS

// ===== PQ gateway frames =====
// Also from the customer's MK4 build: frames a PQ35/46 gateway would normally
// send, for a PQ engine and DSG in a car without the right gateway, so they stop
// logging them as missing. Off by default ("Send PQ Gateway Frames" on the web
// page, saved as "gwEmu" - the customer build's key). Each frame stops by itself
// for a second whenever another module sends that ID, so a real gateway always wins.
// Byte values are the ones the customer's build sends (not checked against a log here).
#define BSG_LAST_ID 0x570         // BSG_Last (power management): ignition / engine running + battery voltage, every 100 ms
#define GATE_KOMF_1_ID 0x390      // Gate_Komf_1 (comfort gateway): reverse gear bit, every 100 ms
#define DIAGNOSE_1_ID 0x7D0       // Diagnose_1 (diagnostic data): mileage, every 500 ms
#define KOMBI_3_ID 0x520          // Kombi_3 from the cluster: the mileage we copy into Diagnose_1 (read only)
#define GATEWAY_FAST_MS 100       // 0x570 and 0x390 go out this often
#define GATEWAY_SLOW_MS 500       // 0x7D0 goes out this often
#define GATEWAY_OTHER_SENDER_MS 1000 // another module sent the ID in this long = stay quiet on it
#define GATEWAY_KM_TIMEOUT_MS 5000   // mileage older than this counts as "not from the cluster" (0 sent)
#define CLUSTER_KM_MAX 999999     // highest believable mileage from the cluster
#define ENGINE_RUNNING_RPM 400    // CAN RPM above this = engine running (0x570 byte 0 bit 7, 13.8 V)
#define GATEWAY_STATE_OFF 0       // web page: gateway frames switched off
#define GATEWAY_STATE_SENDING 1   // web page: we're sending this frame
#define GATEWAY_STATE_OTHER 2     // web page: another module sends it, so we're quiet

// MQB selector lever (Waehlhebel_03, 0x0AF, 10 ms) and the gearbox's reply to
// it (Getriebe_06, 0x128, 20 ms). READ ONLY - shown on the web page.
// The gearbox checks the lever with a question/answer every 40 ms and a
// message counter, so a second module sending 0x0AF breaks that check.
// Can2Cluster never sends these (V3.22-V4.00 did, and that's why it failed).
#define MQB_SELECTOR_ID 0x0AF   // byte 0 top 4 bits = lever position, byte 1 top 4 bits = the same inverted
#define MQB_LEVER_NOT_PRESSED 4 // lever positions (WH_SensorPos_roh, from the K-matrix)
#define MQB_LEVER_D 5           // drive
#define MQB_LEVER_N 6           // neutral
#define MQB_LEVER_R 7           // reverse
#define MQB_LEVER_P 8           // park
#define MQB_LEVER_TIP_PLUS 10   // lever pushed to + in the M gate (Tplus logs)
#define MQB_LEVER_TIP_MINUS 11  // lever pulled to - in the M gate (Tminus logs)
#define MQB_LEVER_S 12          // sport
#define MQB_LEVER_TIP_GATE 14   // lever resting in the M (tiptronic) gate

// Gearbox tip mode (Getriebe_17 0x0B1, byte 2 bits 0-1, GE_TippZustand)
#define MQB_TIP_NONE 0          // normal D/S driving (or P/R/N)
#define MQB_TIP_PERMANENT 1     // lever in the M gate
#define MQB_TIP_TEMPORARY 2     // a paddle was pulled in D/S - proof the gearbox took our 0x3DD

// MQB Getriebe_11 GE_Fahrstufe (gear-lever position) values — byte 5 bits 2..5.
// From the K-matrix, verified against the OpenHaldex MQB logs (see MQB_PADDLES.md).
#define MQB_FAHRSTUFE_INIT 0x0  // between positions / no display
#define MQB_FAHRSTUFE_P 0x5     // park
#define MQB_FAHRSTUFE_R 0x6     // reverse
#define MQB_FAHRSTUFE_N 0x7     // neutral
#define MQB_FAHRSTUFE_D 0x8     // drive
#define MQB_FAHRSTUFE_S 0x9     // sport
#define MQB_FAHRSTUFE_E 0xA     // efficiency (E) - drives like D
#define MQB_FAHRSTUFE_TIP 0xD   // tiptronic from S (paddle pulled in S)
#define MQB_FAHRSTUFE_TIP_D 0xE // tiptronic from D (lever in the M gate, or paddle pulled in D)

// for main functions
extern void basicInit(void);
extern void finishInit(void);
extern void checkError(void *args);
extern void canInit(void);
extern void onBodyRX(const twai_message_t &frame);
extern void needleSweep(void);
extern void startNeedleSweep(void);
extern void needleSweepTask(void *args);
extern void setupPins(void);
extern void blinkLED(int duration, int flashes, bool boolEPC, bool boolEML, bool boolRPM, bool boolSpeed);
extern void updateBlinkLED(void);
extern void broadcastSpeed(void *args);
extern void broadcastPaddles(void *args);
extern void setFrequencySpeed(long frequencyHz);
extern void setFrequencyRPM(long frequencyHz);
extern void incomingHz();
extern void incomingVRHz();
extern void incomingRPMHz();
extern float readHallHz();
extern float readVRHz();
extern float readRPMHz();
extern void resetHallPulseCounter();
extern void resetVRPulseCounter();
extern void resetRPMPulseCounter();

// for EEP
extern void readEEP();
extern void writeEEP();

// for tasks
extern void setupTasks();
extern void showState();
extern void diagTestTask(void *args);

// for buttons (paddles)
extern void padUpFunc(void);
extern void padDownFunc(void);
extern void updateButtons(void);        // read both paddle switches - called every 10 ms by broadcastPaddles()
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