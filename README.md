
# Can2Cluster
Can2Cluster is designed to take CAN messages and convert them into an analog output for use on clusters with traditional pull-down style inputs.  It is geared towards VAG CAN messages but Ford and Emerald ECUs CAN IDs have been added too.

The code is based on an ESP32, and will capture incoming CAN messages and convert them into useable 'analog' signals - like RPM, Speed, Engine Management Light (EML), Electronic Pedal Control Light (EPC) and Reverse (from DSG).  A full breakdown is below.  

Alternative setups and CAN IDs can be added and would allow custom features.

DSG gearboxes are supported and speed is currently calculated using the current gear value and RPM.  KWP2000/UDS would be a neat feature if it can be implemented!

If no speed is available (from either hall or CAN), an optional GPS module (like the Neo6M) can be used to capture speed via. GPS.  This can be broadcast via. CAN if required - users should know the ID they would like it broadcast to.  A reminder that GPS modules need to see the sky and depending on installation location may need an external aerial to be fitted.  It uses a standard connector.

WiFi calibration is supported to minimise code editting and provides valueable feedback on incoming messages.  It is viewable on 192.168.1.1 and is available for 60 seconds after boot! 

**There is a difference in pin-out between Version 1 and Version 2 PCBs - although functionality is the same.  Confirm pinouts using the 'Module Pinout' table.**

This can be expanded to support other marques and is actively encouraged.  Other users are actively using the DSG support and using the EML/EPC outputs to configure shift locks and other features.

![Board Overview](/Images/BoardOverview.png)

### Purchase
If you want to purchase an assembled Can2Cluster, you can do so here: [Can2Cluster - Forbes Automotive](https://forbes-automotive.com/products/can2cluster)

## IO
Can2Cluster uses an 18-pin MX23A18 connector and features the following IO:

#### IO List
| Type | Signal | Notes |
|-----|--------|-------|
| Input | CAN (High & Low) | CANBUS |
| Input | DSG Paddle + | Ground to activate |
| Input | DSG Paddle - | Ground to activate |
| Input | Hall Speed | Pull-up/down options |
| Input | Hall RPM | Pull-down ONLY |
| Input | GPS | Neo6M or similar |
| Output | RPM (MK1/MK2) | High voltage 'coil type' |
| Output | RPM Square | Pull-down ONLY |
| Output | Speed Square | Pull-down ONLY |
| Output | EML & EPC | Pull-down ONLY |
| 11 | Reverse | 5A high-side |

#### Optional / Cool Features 
Shift Light (RPM vs. EML/EPC)
Needle Sweep
Park release for DSG

### Pinout
The MX23A18 connector pinout is:
![Board Overview](/Images/BoardConnector.png)

| Pin/ | Signal | Notes |
|-----|--------|-------|
| 1 | Vign | 12 V Ignition |
| 2 | Ground/MALT | — |
| 3 | Chassis CAN Low | to Chassis/ECU side |
| 4 | Chassis CAN High | to Chassis/ECU side |
| 5 | Speed | Speed output; pull-down |
| 6 | RPM | RPM output; pull-down |
| 7 | EML | Engine Management Light; pull-down |
| 8 | EPC | Electronic Pedal Control Light; pull-down |
| 9 | Reverse | 12v high-side reverse output; 5A max |
| 10 | Paddle Up | For DSG; pull-down |
| 11 | Paddle Down | For DSG; pull-down |
| 12 | Hall Speed | Optional pull-up/down |
| 13 | N/A | Not used |
| 14 | N/A | Not used |
| 15 | N/A | Not used |
| 16 | N/A | Not used |
| 17 | N/A | Not used |
| 18 | RPM | High voltage coil pulse; MK1/MK2 etc |

## Setup
Purchased modules will come pre-loaded with the most recent firmware, check back here for updates.  

> Connect the module as per the wiring diagram

> On initial power up, WiFi is available for the first 60 seconds - if there are no connections it will be turned back off to save power

> Connect to the device and search for '192.168.1.1' in a browser

> Configure the device to suit: Needle Sweep, Sweep Rate, RPM Type, Shift Light

> Use the 'IO' tab to confirm incoming data

### WiFi Setup
Multiple options are available in WiFi:
> Needle Sweep (rate adjustable)

> RPM Output Selection

> Shift light outputs (EML/EPC or both)

> Park Lock Release (for DSG)

> Speed Selection (Hall/ECU/ABS/DSG/GPS)

![WiFi1](/Images/WiFi1.jpg)
![WiFi2](/Images/WiFi2.jpg)
![WiFi3](/Images/WiFi3.jpg)
![WiFi4](/Images/WiFi4.jpg)
![WiFi5](/Images/WiFi5.jpg)
![WiFi6](/Images/WiFi6.jpg)
![WiFi7](/Images/WiFi7.jpg)

## Adding CAN Functionality
Users wishing to add CAN functionality not available are encourged to push commits to the project.  The main section of code for managing imcoming CAN messages are handled in an interrupt and pushed automatically.  Users should know the CAN ID and Bytes they wish to process and what variables to assign them to.

For example (in _CAN.ino under "onBodyRX");
```
    case emeraldECU1_ID:
      vehicleRPM = ((frame.buf[0] << 8) | frame.buf[1]);  // conversion: 0.25*HEX // this is RPM
      break;

    case emeraldECU2_ID:
      vehicleSpeed = ((frame.buf[2] << 8) | frame.buf[3]) * (2.25 / 256);  // conversion: 0.25*HEX // this is RPM
      break;

```
The above entry will check to see if there is a CAN message from "0x1000" or "0x1001" (Emerald ECU 1 & 2) and then parse the data to the relevant variables.

## Adding GPS
Users wishing to add GPS at a future date can do - the boards are already socketed in readiness.  The 4-pin JST XH Connector will provide 3.3v, RX, TX and ground.

![GPS Connection](/Images/BoardGPS.png)

## Jumpers
To keep functionality, there are a variety of jumpers available on the board which can be added/removed to suit the chassis.

![Jumpers](/Images/Jumpers.png)

### R-Term
The jumper marked 'r-term' is the terminating resistor for the CANBUS network.  If there are no other CAN devices on the network (this is the only one), the jumper should remain.  If there are other devices on the network, this can be removed.

### SpeedPulser
A selector for a hall sensor can be configured as pull-up or pull-down with the central pin being the hall sensor itself.  Typically, VAG ones are 'pull-up'.

### 12v Pullup
Used to configure outputs if the cluster does not have internal pull-ups.

### Over-the-Air Updates
OTA has now been enabled - so the updating to the latest release will allow future updates to be seemless.  

> Download the most recent 'Release' - this will be saved locally on your phone/laptop/etc as a '.bin'.
> Connect to Can2Cluster V2 on WiFi
> Go to 192.168.1.1/update
> Select file - locate the new '*.bin'
> Wait for ESP to reboot

## PCB Design
The PCB has been designed in EasyEDA and available in the 'PCB' folder.

## PCB Principles
The board is based around an ESP32 with an external CAN chip (SN65HVD230) with an optional termination resistor (if there are no other CAN devices on the network). 

The outputs are controlled via. a ULN2003A (a 200mA 7-channel Darlington Array) which is socketed so that if damaged can be swapped out.  This array is used to trigger each of the outputs - including the high-side 5A MOSFET (for reverse light, typically).

GPS is captured via. a Neo6M module and via. a Software Serial connection.  It is supplied with an external aerial - although there may be a requirement to add a cabled external aerial.  The connector is a standard SMA connector.  

The aerial that can be found on Google with a search "sma gps antenna".

![Board Top](/Images/BoardTop.png)
