
# Can2Cluster

**Get one ready-built:** [Can2Cluster at Forbes Automotive](https://forbes-automotive.com/products/can2cluster?utm_source=github&utm_medium=readme&utm_campaign=can2cluster) · Pair it with a [SpeedPulser](https://forbes-automotive.com/products/speedpulser?utm_source=github&utm_medium=readme&utm_campaign=can2cluster) for cable speedos · Not sure which board you need? See the [speedo, tacho & cluster comparison](https://forbes-automotive.com/collections/speedo-tacho-cluster?utm_source=github&utm_medium=readme&utm_campaign=can2cluster).
Can2Cluster takes CAN messages and turns them into the analogue signals older instrument clusters expect: RPM, speed, Engine Management Light (EML), Electronic Pedal Control light (EPC), reverse and a coolant gauge. It is built around VAG CAN messages (PQ and MQB), with Ford, Emerald and Renault ECUs added too, and runs on an ESP32 DevKit board or the newer ESP32-S3 board from the same code.

DSG gearboxes are supported: speed can be worked out from the current gear and RPM, or read over TP2.0 / UDS. The DSG paddle inputs can shift an MQB or PQ gearbox over CAN, or, on a PQ car with an MK4 cruise stalk, the same inputs can carry the cruise stalk to the ECU.

If no speed is available (from hall or CAN), an optional GPS module (like the Neo6M) can provide it, and the speed can be broadcast on CAN for other modules. GPS modules need to see the sky; depending on where the unit is fitted, an external aerial may be needed (standard SMA connector).

Everything is set up from a web page on the unit's own WiFi, so no code editing is needed, and it shows live feedback on what's coming in.

> [!NOTE]
> The pin-out differs between the Version 1 and Version 2 PCBs, although functionality is the same. Confirm pin-outs using the 'Module Pinout' table.

This can be expanded to support other marques and is actively encouraged. Other users are using the DSG support and the EML/EPC outputs for shift lights and other features.

![Can2Cluster Web UI: dashboard, needle sweep, MK4 cruise stalk, speed & RPM sources, status, live inputs, output tests and OTA](/Images/can2clusterUI.png)

![Can2Cluster Web UI: DSG paddles, PQ gateway frames, DSG speed calculation, GPS & speed broadcast, CAN analyzer, coolant gauge and board diagnostics](/Images/can2clusterUI-2.png)

![Board Overview](/Images/BoardOverview.png)

### Purchase
If you want to purchase an assembled Can2Cluster, you can do so here: [Can2Cluster - Forbes Automotive](https://forbes-automotive.com/products/can2cluster)

## Features
| Feature | What it does |
|---|---|
| RPM output | Square wave or high-voltage 'coil type' (MK1/MK2) RPM, from CAN or a hall RPM input |
| Speed output | Square wave speed from hall, VR, ECU, ABS, DSG, TP2.0/UDS, GPS or a custom CAN ID |
| EML / EPC / Reverse | Warning lights and a 5A reverse output from CAN |
| Needle sweep | Sweeps both needles at power-up, as soon as the outputs are ready, up to a settable speedo dial top |
| Shift light | Flashes EML and/or EPC above a set RPM |
| DSG park | Shows Park on the EML or EPC light |
| DSG paddles | Paddle inputs sent to an MQB (0x3DD) or PQ (0x38A) gearbox |
| MK4 cruise stalk | The paddle and hall inputs read an MK4 cruise stalk and send it to a PQ ECU (0x38A) |
| PQ gateway frames | Optional 0x570 / 0x390 / 0x7D0 for a PQ engine and DSG with no gateway |
| Coolant gauge | Drives an MK2-style coolant gauge from the CAN coolant temperature, with a calibration builder |
| GPS + speed broadcast | GPS speed, and any speed put back onto CAN on an ID of your choosing |
| CAN analyzer | Every frame forwarded to SavvyCAN over WiFi or USB |
| OTA updates | Firmware and web page updates from GitHub or from files, no USB needed |

## IO
Can2Cluster uses an 18-pin MX23A18 connector and features the following IO:

#### IO List
| Type | Signal | Notes |
|-----|--------|-------|
| Input | CAN (High & Low) | CANBUS |
| Input | DSG Paddle + / Cruise Main Switch | Ground to activate |
| Input | DSG Paddle - / Cruise SET | Ground to activate |
| Input | Hall Speed / Cruise RESUME | Pull-up/down options |
| Input | Hall RPM | Pull-down ONLY |
| Input | GPS | Neo6M or similar |
| Output | RPM (MK1/MK2) | High voltage 'coil type' |
| Output | RPM Square | Pull-down ONLY |
| Output | Speed Square | Pull-down ONLY |
| Output | EML & EPC | Pull-down ONLY |
| Output | Reverse | 5A high-side |

### Pinout
The MX23A18 connector pinout is:
![Board Overview](/Images/BoardConnector.png)

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | Vign | 12V Ignition |
| 2 | Ground | Chassis ground |
| 3 | Chassis CAN Low | to Chassis/ECU side |
| 4 | Chassis CAN High | to Chassis/ECU side |
| 5 | Speed | Speed output; pull-down |
| 6 | RPM | RPM output; pull-down |
| 7 | EML | Engine Management Light; pull-down |
| 8 | EPC | Electronic Pedal Control Light; pull-down |
| 9 | Reverse | 12v high-side reverse output; 5A max |
| 10 | Paddle Up / Cruise Main | DSG paddle +, or the MK4 cruise main switch in cruise stalk mode; ground to activate |
| 11 | Paddle Down / Cruise SET | DSG paddle -, or cruise SET in cruise stalk mode; ground to activate |
| 12 | Hall Speed / Cruise RESUME | Hall speed sensor, or cruise RESUME in cruise stalk mode; optional pull-up/down |
| 13 | N/A | Not used |
| 14 | N/A | Not used |
| 15 | N/A | Not used |
| 16 | N/A | Not used (S3 board: coolant gauge output) |
| 17 | N/A | Not used |
| 18 | RPM | High voltage coil pulse; MK1/MK2 etc |

> [!NOTE]
> **Legacy clusters & needle jitter:** on some older clusters the coil-style RPM output can induce a little jitter in the hall inputs due to poor grounding. Fitting a **~500 Ω resistor in series** with the RPM output largely reduces this.

## Setup
Purchased modules come pre-loaded with the most recent firmware; check back here for updates.

1. Connect the module as per the wiring diagram.
2. Power up. WiFi is available straight away and stays on while any client is connected; once the last one disconnects, the radio turns off after 1 minute to save power (see Power Management below).
3. Connect to the device and browse to **192.168.1.1**.
4. Configure the device to suit: needle sweep and dial top, RPM type, speed source, shift light, input mode.
5. Use the **Diagnostics** tab to confirm incoming data.

### WiFi Setup
Connect to the **`Can2Cluster`** access point (open network, no password) and browse to **192.168.1.1** or **c2c.local**. The web page uses Forbes Automotive's shared dark theme, the same look as OpenHaldex, SpeedPulser, SpeedPulserPro, can2rpm, the MQB Steering Wheel Controller and AirLift Controller. Current firmware version: **4.03**.

The web page has five tabs:
- **Dashboard**: final RPM and speed (as numbers or dials) and the status pills.
- **Configuration**: needle sweep, shift light, cluster output (coil RPM, MPH), DSG park, input mode (paddles or MK4 cruise stalk), PQ gateway frames and DSG paddle output.
- **Advanced**: speed broadcast, GPS update rate, CAN analyzer, speed and RPM sources, custom CAN input, DSG speed calculation and the coolant gauge.
- **Diagnostics**: board details, every input side by side, and the output tests.
- **OTA**: updates from GitHub or from files, and Home WiFi (bridge mode).

<p align="center">
  <img src="/Images/ui-dashboard.png" alt="Dashboard: RPM and speed gauges, system status pills and display options" width="260">
  <img src="/Images/ui-needle-sweep.png" alt="Configuration: needle sweep enable, speedo dial top, sweep speed and RPM / speed step" width="260">
  <img src="/Images/ui-shift-light.png" alt="Configuration: shift light output, RPM limit and flashes; cluster output coil / MPH options; DSG park behaviour" width="260">
</p>
<p align="center">
  <img src="/Images/ui-speed-rpm.png" alt="Advanced: speed and RPM source selection with hall / VR frequency limits" width="260">
  <img src="/Images/ui-dsg-speed.png" alt="Advanced: DSG speed calculation with gear ratios, final drives and tyre circumference" width="260">
  <img src="/Images/ui-system-status.png" alt="Dashboard: system status pills for CAN bus, GPS, EML, EPC, reverse, park and paddles" width="260">
</p>

*Dashboard with the final RPM/speed as dial gauges and the status pills; needle sweep, shift light, cluster output and DSG park on the Configuration tab; speed and RPM source selection under Advanced. Choosing **DSG** as the speed source reveals the gear-ratio calculator (speed = RPM × tyre circumference ÷ gear ratio × final drive).*

**Status indicators:** CAN health, GPS and output (EML/EPC/Reverse) status are shown as coloured pills:

| Colour | Meaning |
|---|---|
| Green | Healthy / active: CAN bus is healthy, GPS has a satellite lock, or the output reflects a real vehicle CAN signal |
| Red | Unhealthy / not available: CAN bus unhealthy, or no GPS module fitted/connected |
| Orange | A caveat, not a failure: GPS module connected but no satellite fix yet ("Connected (no sats)"), or an EML/EPC/Reverse output **forced on by the test/diagnostic mode** rather than a real vehicle signal |

### Needle Sweep
At power-up both needles sweep to the top of the dial and back. The sweep starts as soon as the output pins are set up, a fraction of a second after power-on, and runs on its own while the rest of the unit starts up (GPS search, CAN, WiFi), so there's no wait before it.

**Speedo Dial Top** sets how far the speed needle goes: set it to the highest number on the speedo (60 to 400, km/h or mph to match the cluster). Up to V4.01 the sweep always stopped at 200. **Sweep Speed**, **RPM Step** and **Speed Step** set how long the sweep takes (higher = slower); **Test Needle Sweep** runs one from the page.

## DSG Paddles
Pulling the DSG Paddle +/- inputs puts a paddle request on the CAN bus, whatever the speed source. The **DSG Paddle Output** drop-down picks the frame: **MQB** sends Gateway_74 (`0x3DD`), the frame an MQB gearbox reads its steering-wheel paddles from; **PQ** sends the GRA frame (`0x38A`); **PQ + MQB** sends both; **Off** sends nothing. Pick the one that matches the gearbox: a PQ gearbox only reads `0x38A` and an MQB one only `0x3DD`, so the wrong choice does nothing. Each pull sends **one clean tip**: a short pulse on the paddle's rising edge, then the bit is released, with a brief lock-out before the next shift. That is how the gearbox actually takes a tip (edge-triggered, one shift per press, with a minimum gap between shifts) - worked out by reverse-engineering the DSG firmware, write-ups in [Documents/dsg_re/](Documents/dsg_re/). Holding a paddle gives exactly one shift. If a real gateway is already sending that frame, Can2Cluster stays quiet on it. The MQB gear selector's own frame (`0x0AF`) is only read, never sent: the gearbox checks the lever with a question/answer and a message counter, so it can't be imitated while the real lever is fitted. The card also shows the lever position, the gearbox's tip mode, gear and warning level live, and has **Test Paddle +/-** buttons. The full log analysis is in [MQB_PADDLES.md](MQB_PADDLES.md).

## MK4 Cruise Stalk
For a PQ engine in a car with the MK4 cruise stalk. **Input Mode** (Configuration tab) picks what connector pins 10, 11 and 12 are used for:

| Pin | DSG Paddles (default) | MK4 Cruise Stalk |
|---|---|---|
| 10 | Paddle + | Main switch |
| 11 | Paddle - | SET |
| 12 | Hall speed | RESUME |

In cruise stalk mode each input is on when grounded, and the stalk goes to the ECU in the PQ cruise frame GRA (`0x38A`) every 20 ms: main switch, SET and RESUME, plus "cancel" for a moment when the main switch is switched off. The paddle bits in that frame are always 0, so the stalk can never shift the gearbox, and no paddle frames are sent. Hall speed isn't available in this mode (pin 12 is RESUME), so pick another speed source. The card shows each stalk input and what's being sent live; check every stalk position there before driving. Cruise also has to be enabled in the ECU coding, and the ECU must see both brake switches.

> [!WARNING]
> Match the setting to the wiring. In paddle mode, grounding pin 10 or 11 shifts the gearbox, so never select DSG Paddles while the cruise stalk is wired to those pins. The page asks for confirmation before switching.

**PQ Gateway Frames** (off by default) is for a PQ35/46 engine and DSG in a car without the right gateway. It sends the frames a gateway would normally send, so the ECU and DSG stop logging them as missing: `0x570` (ignition on / engine running and the battery voltage) and `0x390` (reverse gear) every 100 ms, and `0x7D0` every 500 ms with the mileage read from the cluster (`0x520`). Each frame stops by itself while another module sends it, so leave it off on cars that have a gateway.

The cruise stalk and gateway frames were first built by a customer for their own car (an MK4 build of 4.01) and are now part of the standard firmware. Their settings carry across when updating from that build.

<p align="center">
  <img src="/Images/ui-cruise.png" alt="Configuration: Input Mode set to MK4 Cruise Stalk, with main switch, SET, RESUME and what's being sent on 0x38A" width="260">
  <img src="/Images/ui-gateway.png" alt="Configuration: PQ Gateway Frames with the state of 0x570, 0x390, 0x7D0 and the mileage read from the cluster" width="260">
  <img src="/Images/ui-paddles.png" alt="Configuration: DSG Paddle Output frame choice, live paddle / lever / tip mode / gear readout and test buttons" width="260">
</p>

## Adding CAN Functionality
Users wishing to add CAN functionality are encouraged to push commits to the project. Incoming CAN messages are handled in `onBodyRX()` (`can2cluster_can.cpp`) and the outputs follow automatically. You'll need to know the CAN ID and bytes you want to process and which variables to put them in.

Two things in the web page help here. **CAN Analyzer** (Advanced tab) forwards every received frame to [SavvyCAN](https://www.savvycan.com/) over WiFi (GVRET, `192.168.1.1:23`) or over the USB serial port, so you can find the ID and bytes you're after. And for a speed signal on a non-standard ID, **Custom CAN Input** (choose *Custom* as the speed source) lets you enter the CAN ID, byte positions, byte order, scale and offset without touching the code:

<p align="center">
  <img src="/Images/ui-can-analyzer.png" alt="Advanced: CAN Analyzer, forwarding frames to SavvyCAN over WiFi or serial" width="260">
  <img src="/Images/ui-custom-can.png" alt="Advanced: Custom CAN Input with CAN ID, byte indexes, byte order, scale and offset, and the parsed speed shown live" width="260">
  <img src="/Images/ui-gps.png" alt="Advanced: Broadcast Speed (CAN ID, DLC, byte layout, scale) and GPS Update Rate" width="260">
</p>

For example (in `can2cluster_can.cpp`, `onBodyRX()`):
```
  case emeraldECU1_ID:
    vehicleRPMCAN = ((frame.data[0] << 8) | frame.data[1]); // this is RPM
    break;

  case emeraldECU2_ID:
    calcSpeed = ((frame.data[2] << 8) | frame.data[3]) * (2.25 / 256); // this is speed
    break;
```
The above checks for a CAN message from `0x1000` or `0x1001` (Emerald ECU 1 & 2) and puts the data into the matching variables.

## Adding GPS
GPS can be added later; the boards are already socketed for it. The 4-pin JST XH connector provides 3.3V, RX, TX and ground.

![GPS Connection](/Images/BoardGPS.png)

Once fitted, pick **GPS** as the speed source on the Advanced tab. The same tab sets the receiver's **update rate** and, if another module on the bus needs the speed, **Broadcast Speed** puts it onto CAN on an ID, byte layout and scale of your choosing (shown above).

## Hall & RPM Input Parsing
When speed or RPM comes from the hall/VR/pulse inputs (rather than CAN), the frequency is measured over a **window** rather than from a single edge-to-edge period. Each interrupt adds up the edge intervals and a count, and the task averages them once per loop (frequency = intervals ÷ summed interval time), which gives a true average over that window.

This matters because a single-period reading turns tone-wheel / tooth-spacing jitter straight into a jumpy needle. Pulses closer together than ~3 ms (> 333 Hz) are rejected as electrical noise, so ignition-coil interference on the input can't be mistaken for a real speed or RPM. **Max Hall / VR Sensor Frequency** is the sensor's frequency at 200 km/h; **Cluster Frequency Limit** and **Cluster RPM Limit** set the RPM scale.

Every input the module can see is listed side by side on the **Diagnostics** tab, so it's obvious which sources are live before choosing one, and the output tests drive the cluster directly from the page:

<p align="center">
  <img src="/Images/ui-live-inputs.png" alt="Diagnostics: live data from every input (RPM, hall, CAN, VR, ECU, ABS, DSG, TP2.0, UDS, GPS and custom CAN speed)" width="260">
  <img src="/Images/ui-output-tests.png" alt="Diagnostics: speed and RPM output tests, EML / EPC / reverse output tests and diagnostic mode" width="260">
  <img src="/Images/ui-board-diag.png" alt="Diagnostics: board details" width="260">
</p>

*Diagnostics: every speed and RPM source side by side; slider-driven speed and RPM output tests (the speed test goes up to the speedo dial top), the EML / EPC / reverse output tests and the diagnostic (forced-output) mode that shows as orange pills on the dashboard; and the board details. On the S3 board this card also shows the coolant DAC (MCP4725), the board ID, CAN termination, output pull-ups and the speed input bias.*

## Coolant Gauge Output
The MK2-style coolant temperature gauge can be driven from the CAN coolant temperature. On the DevKit board it uses one of the ULN2003 outputs (EML or EPC; selecting one takes that pin away from the shift light / DSG park). On the S3 board it has its own output (connector pin 16) and follows the calibration curve. At or above the **warning temperature** the gauge is pegged and the cluster warning light is triggered.

Because every gauge is different, the **Calibration Builder** lets you drive the needle directly: enable calibration mode, jog the duty until the needle sits on a known mark, pick that temperature and **Capture**. The stored points are drawn as a curve and interpolated at runtime.

<p align="center">
  <img src="/Images/ui-coolant.png" alt="Advanced: Coolant Gauge Output with output pin, warning temperature, live coolant temperature and applied duty" width="260">
  <img src="/Images/ui-coolant-cal.png" alt="Advanced: Calibration Builder with duty jog buttons and captured points" width="260">
</p>

## Jumpers
To keep functionality, there are a variety of jumpers on the board which can be added/removed to suit the chassis.

![Jumpers](/Images/Jumpers.png)

### R-Term
The jumper marked 'r-term' is the terminating resistor for the CANBUS network. If there are no other CAN devices on the network (this is the only one), the jumper should stay. If there are other devices on the network, it can be removed.

### SpeedPulser
Selects pull-up or pull-down for a hall sensor, with the central pin being the hall sensor itself. Typically VAG ones are 'pull-up'. For the MK4 cruise stalk's RESUME button on pin 12, use pull-up.

### 12v Pullup
Used to configure outputs if the cluster does not have internal pull-ups.

## Over-the-Air Updates
Everything happens on the **OTA** tab; USB is only needed for the very first install, or if the web interface is ever lost.

- **Update from GitHub**: press **Check for updates**. The page reads the release list from [`adamforbes92/can2cluster`](https://github.com/adamforbes92/can2cluster) (`Releases/releases.json` plus the `V…` folders), shows *Installed* vs. *Latest* (older and beta releases sit behind a tick-box, for rolling back), then installs the selected version in six tracked steps: download and flash the web UI, verify it, download and flash the firmware, reboot. Each board is only offered its own firmware. The browser needs internet *while it can still reach the controller*: join the controller to your router in the **Home WiFi (Bridge Mode)** card, put the phone on the same network and open the address the card shows (`http://c2c.local/` works there too). On the controller's own access point alone most phones drop mobile data.
- **Home WiFi (Bridge Mode)**: the controller joins a home or garage network as a station alongside its own access point. Scan or type the network name, save, and once it shows *Connected* the controller is reachable from that network as well. It tries hard for 20 s after boot, then once every 5 minutes, so a saved network can't disturb the access point when the car is parked away from home.
- **Update from Files**: no internet needed. Download `littlefs.bin` and `firmware.bin` from the newest `Releases/V…/` folder and upload them here, **Filesystem** (web UI) first, then **Firmware**. The device reboots after the firmware.

Every upload is size-checked, and a filesystem image is verified by mounting it before the device reports success; a failed filesystem upload is wiped rather than left half-written. If the web interface is ever missing (a filesystem update that stopped part-way), the controller serves a **recovery page** at its address with the same two uploads and partition diagnostics, so it is never a USB job. Web files are served `no-cache`, so a new UI is never hidden behind a stale copy in the phone's browser.

<p align="center">
  <img src="/Images/ui-ota.png" alt="OTA tab: Update from GitHub, Home WiFi bridge mode and Update from Files" width="260">
</p>

## Power Management
The firmware includes a reduced-power module that saves current once no clients are connected to the WiFi access point. This directly reduces heat from the on-board linear regulator, which turns `(Vin − Vout) × current` into heat, so every milliamp saved is less heat.

**1 minute after the last client disconnects:**
- WiFi radio off: the single biggest saving (~80–120 mA average)
- CPU scaled from 240 MHz to 80 MHz

Bluetooth is released at boot (frees ~60 KB RAM) and the onboard LED is switched off. Everything comes back the moment a client reconnects, and a power-cycle (ignition off/on) also brings WiFi back.

## PCB Design
The PCB has been designed in EasyEDA and is in the 'PCB' folder.

## PCB Principles
The DevKit board is based around an ESP32 with an external CAN chip (SN65HVD230) and an optional termination resistor (if there are no other CAN devices on the network).

The outputs are driven through a ULN2003A (a 200mA 7-channel Darlington array), socketed so it can be swapped if damaged. It also triggers the high-side 5A MOSFET (typically for the reverse light).

GPS is read from a Neo6M module on a hardware serial port. It comes with an aerial, although a cabled external aerial may be needed; the connector is a standard SMA ("sma gps antenna" on Google finds them).

The S3 board uses an ESP32-S3-WROOM-1-N16R8 with native USB, a TCAN1044 CAN transceiver with switchable termination, a TPL7407 output driver, switchable 12V pull-ups and a dedicated coolant gauge output (MCP4725 DAC driving a current sink).

![Board Top](/Images/BoardTop.png)

## Version History
- **4.03**: DSG paddles reworked and the MQB DSG speed read fixed, after reverse-engineering the gearbox firmware (write-ups in [Documents/dsg_re/](Documents/dsg_re/)). Each pull now sends one clean tip (a short pulse on the rising edge, then released, with a lock-out before the next) instead of holding the bit, which stops the odd double gear-jump and the need to press twice. MQB DSG speed now read over UDS as DID `0x100A` (km/h = raw/10) on `0x7E1/0x7E9`, replacing a wrong placeholder; PQ DSG speed (TP2.0, raw/3) confirmed correct and unchanged.
- **4.02**: MK4 cruise stalk input mode (pins 10/11/12 to a PQ ECU on 0x38A) and optional PQ gateway frames (0x570 / 0x390 / 0x7D0), both from a customer's MK4 build; needle sweep starts straight away instead of after the GPS search; settable Speedo Dial Top for the sweep (was fixed at 200).
- **4.01**: MQB paddles rebuilt from a full log review (Gateway_74 0x3DD, held for as long as the paddle), selector emulation removed, paddle readout and test buttons on the web page.
- **4.00**: one codebase for the DevKit and the new ESP32-S3 board (dedicated coolant gauge output).
- **3.24**: OTA overhaul (Update from GitHub, Home WiFi bridge mode, recovery page); speed/RPM outputs moved to hardware timers.
- **3.23**: shared Forbes Automotive web theme.
- **3.20 - 3.22**: MQB support, coolant gauge output, aftermarket speed input, Renault Megane RPM, MQB paddles.
- **3.10**: power management (WiFi off when idle).
- **3.00 - 3.02**: OTA, GPS rework, MPH clusters, SavvyCAN analyzer.
- **2.00 - 2.01**: TP2.0 / UDS DSG speed, PlatformIO port, GPS update rate.

The full history is in `include/can2cluster_ver.h`.
