
# Can2Cluster

Geared towards VAG CAN messages, Can2Cluster is designed to take CAN messages and convert them into an analog output for use on clusters.

Based on an ESP32, this will capture incoming CAN messages and convert them into useable signals - like RPM, Speed, Engine Management Light (EML), Electronic Pedal Control Light (EPC), Reverse.  A full breakdown is below.

DSG gearboxes are supported and speed is currently calculated using the current gear value and RPM.  If no speed is available (from hall or CAN), an optional GPS module (like the Neo6M) can be used to capture speed via. GPS.  This can be broadcast via. CAN if required.

WiFi calibration is supported and provides valueable feedback on incoming messages.  It is viewable on 192.168.1.1 and is available for 60 seconds after boot!

**There is a difference in pin-out between Version 1 and Version 2 PCBs - although functionality is the same.  Confirm pinouts using the 'Module Pinout' table.**

This can be expanded to support other marques and is actively encouraged.  

Inputs:

## IO
Can2Cluster uses an 18-pin MX23A18 connector and features the following IO:

#### Inputs

```http
  CAN (High and Low)
  Paddle Up (for DSG)
  Paddle Down (for DSG)
  Hall Sensor (12v Square/Frequency)
  GPS
```
#### Outputs 

```http
RPM (as high-voltage)
RPM
Speed
EML (200mA max.)
EPC (200mA max.)
Reverse (5A max.)
```
#### Optional / Cool Features 

```http
Shift Light (RPM configurable)
Needle Sweep
```