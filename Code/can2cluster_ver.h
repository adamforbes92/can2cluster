/*

V1.00 - Optional 'traditional' coil output
V1.01 - Optional EML/EPC output.  EPC can be used as 'shift light', RPM configarble
V1.02 - Original RPM input is ~500Hz, speed is ~300Hz for VW Clusters.  Adjustable in code
V1.03 - Optional GPS module for calculating speed if ECU is blind.  Not as accurate but a valid solution...
V1.03 - Built-in LED used for error displaying.  For example - no satellites will illuminate LED
V1.04 - Added DSG support - gets current gear & rpm and calculates theory speed.  Ratios in '_dsg.ino'
V1.05 - Check for hanging
V1.06 - Slowed dowm RPM to minimise speed change during shift
V1.07 - calibrated PWM motor 
V1.08 - added DSG reverse specifics
V1.09 - added WiFi
V1.10 - tested and added selectors for input type on WiFi rather than switches.  Overall cleanup
V1.11 - added Ford details - thanks to Jamie(!)
V1.12 - added MPH conversion
V1.13 - added DSG Up/Down paddles & DSG gear logging
V1.14 - changed button library to ESP32 Interrupt Button to make paddles faster
V1.15 - moved everything to tasks
V1.16 - added test output functionaility - for Reverse/EML/EPC etc
V1.17 - revised needle sweep maths so it is smoother / race conditions not met
V1.18 - added an error on GPS wiring
*/