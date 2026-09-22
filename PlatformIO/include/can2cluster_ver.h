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

V2.00 - added TP/UDS querying for DSG speed (for models that don't broadcast it on the CAN but support it via. diagnostics).  PlatformIO port.
V2.01 - added GPS update rate configuration 

V3.00 - revised GPS to match SpeedPulserPro and added OTA with filesystem AND firmware
V3.01 - added cluster in MPH and SavvyCAN implementation for CAN analysis (TCP:23 or Serial 1Mbaud)
V3.02 - changed rpm/speed update rate to ensure GPS has a chance to update and revised ALL Serial bug updates

V3.10 - added power management - turning WiFi off after 1 minute of inactivity.  Reboot to enable.  Cuts power from 300mA idle to 170mA and runs cooler.
    * board to be revised with a buck converter but space/redesign etc.


V3.20 - added MQB support. Added TP2.0 and UDS speed support for DSG speed.  Added aftermarket speed input for non-VW vehicles
      - added coolant temp output.  Uses a PWM signal to 'drive' the cluster gauge.  Can be calibrated via. WiFi
      - Added a PWM frequency limit to avoid high-temp LED lighting

V3.21 - added Renault Megane RPM input

V3.22 - added MQB DSG paddle up/down emulation on the real shifter CAN ID (0x0AF),
        reverse-engineered from CrazyQuiffs' tip+/tip- CAN logs captured directly
        at the shifter/paddle module

V3.23 - standardised Forbes Automotive UI theme (shared style.css);
        adopted common wifi_manager (mDNS: can2rpm.local) and ota_manager
        (firmware + filesystem OTA via /api/ota, /api/ota/fs); per-product
        cache-busting on web assets
V3.24 - OTA overhaul (shared ota_manager / wifi_manager v2 + data/ota.js, ported
        from OpenHaldex 9.00): upload callbacks no longer answer mid-body (the
        old per-chunk "200 OK" made the browser drop the connection after the
        first 1.4 kB - a crash in AsyncTCP and a half-written partition, so no
        OTA through the UI had ever completed); filesystem updates unmount
        first, check the announced size, verify the mount and wipe on failure;
        boot only mounts a sane superblock and the web server always starts -
        with no usable UI "/" is a recovery page with the two uploads.
        "Update from GitHub" on the OTA tab (Releases/releases.json via
        tools/make_release.py) plus a Home WiFi (bridge mode) card; power_manager
        holds WiFi up while any browser is active. Assets served no-cache (ETag)
        instead of the hand-bumped ?v=.

*/