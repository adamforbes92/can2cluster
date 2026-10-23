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
        Speed and RPM outputs moved from LEDC to GPTimer square waves: on
        Arduino-ESP32 3.3.x / IDF 5.5.2 the LEDC duty update spins with
        interrupts off until the previous update latches (one timer period on
        the ESP32), which at 1-2 Hz trips the 300 ms interrupt watchdog and
        reboots the board. The 5 kHz "latch" workaround also pinned the
        low-speed LEDC block to APB, so nothing under ~76 Hz was reachable.
        Coolant PWM stays on LEDC (10 Hz min) but re-selects its clock source
        on a frequency change instead of ledc_set_freq().

V4.00 - two boards from one codebase. platformio.ini has two envs:
          esp32doit-devkit-v1 (-DBOARD_DEVKIT=1) - the original DevKit board,
            unchanged: coolant gauge still borrows the EML or EPC output as PWM.
          esp32-s3 (-DBOARD_S3=1) - the new ESP32-S3-WROOM-1-N16R8 board
            (PCB/ProPrj_can2clusterV4-s3-reva_2026-10-03). Pins taken from the
            PCB netlist (see can2cluster_defs.h). Native USB for Serial/upload,
            16 MB partition table.
        S3 coolant gauge: dedicated COOLANT_OUT (J1 pin 16), MCP4725 DAC ->
            op-amp current sink. Follows the calibration curve (real needle
            position, not just on/off); pegged at/above the warning temperature.
            EML/EPC stay free for their lights.
        S3 board switches set at boot: CAN transceiver standby (normal), CAN
            termination (off), 12V output pull-ups (on), hall speed input bias
            (12V pull-up). Set in can2cluster_defs.h for now.
        Removed the never-built "new DevKit board" TCA9554 I/O-expander path
            (isNewBoard / GPIO32 board sense). EML/EPC/Reverse/paddles are plain
            pins on both boards.
        OTA: shared ota_manager / ota.js / make_release.py updated to the
            board-aware _SharedTheme version - each board is only offered its
            own firmware from GitHub (Releases/V4.00/ and Releases/V4.00-S3/).
        Web page: one data/ folder for both boards; board-only options are
            shown/hidden from the "board" field in /api/settings.

V4.01 - MQB paddles rebuilt after a full review of the Tplus/Tminus logs against
        the MQB K-matrix (write-up: MQB_PADDLES.md).
        The logs are the gear LEVER's tip gate (0x0AF), engine off - not paddles.
        0x0AE is Getriebe_12 (gearbox -> engine): its payload never changed.
        MQB paddles now go out the factory way: Gateway_74 (0x3DD) byte 7,
            every 100 ms plus straight away on a change, held for as long as
            the paddle is held (150 ms minimum). Stays quiet if a real gateway
            is already sending 0x3DD.
        Removed the selector (0x0AF) emulation: the gearbox checks the lever
            with a question/answer and a counter, so a second sender on 0x0AF
            can't work alongside the real lever.
        Paddles read as held / not held (OneButton click only fired on release,
            and never for a pull longer than 0.8 s). Read by the paddle task
            every 10 ms instead of loop().
        Paddle output no longer needs DSG as the speed source; new "Off" choice
            (the default). Old "paddleOutput" setting carried across once.
        Getriebe_11: GE_Fahrstufe 14 (tip from D) and 10 (E) were missing, so
            the gear (and DSG speed) froze in the M gate / when using paddles.
        Web page: live paddle readout (sending / lever / tip mode / gear /
            gearbox warning) and Test Paddle +/- buttons. Paddle frames we send
            are copied into the SavvyCAN analyzer stream.

V4.02 - MK4 cruise stalk and PQ gateway frames, rebuilt from a customer's own
        MK4 build (v4.01.2, binaries only - worked out from its firmware and web page).
        New "Input Mode" (Configuration tab): pins 10/11/12 are either the DSG
            paddles + hall speed (as before, the default) or the MK4 cruise
            stalk: pin 10 main switch, pin 11 SET, pin 12 RESUME, sent to a PQ
            ECU in GRA_Neu (0x38A) every 20 ms. Switching the main switch off
            sends cancel for 200 ms. Paddle bits always 0 in this mode, no 0x3DD,
            no hall speed. Saved under the customer build's key ("cruiseStalk").
        New "Send PQ Gateway Frames" (off by default): 0x570 BSG_Last and 0x390
            Gate_Komf_1 every 100 ms, 0x7D0 Diagnose_1 every 500 ms with the
            mileage read from the cluster's Kombi_3 (0x520). Each goes quiet for
            a second whenever another module sends that ID. Key "gwEmu".
        Needle sweep starts as soon as the outputs are set up, in its own task,
            instead of after the GPS search (3 s with no GPS fitted) and the
            500 ms settle delay. The rest of start-up carries on during the sweep.
        New "Speedo Dial Top" setting (60-400, default 200): where the sweep
            takes the speed needle. Up to V4.01 it was fixed at 200. The hall /
            VR frequency sliders still mean "Hz at 200 km/h", so no speed
            calibration moves.
        Web page: RPM / Speed Step sliders now 1-50 (anything above 50 was reset
            at boot); the Speed Output Test slider goes up to the dial top.
        platformio.ini: no per-project build_dir (the desktop's
            PLATFORMIO_WORKSPACE_DIR handles it).

V4.03 - DSG paddles reworked and the MQB speed read fixed, after reverse-
        engineering the DSG mechatronic firmware (DQ250 02E, DQ500 0BH, DQ250-MQB
        0D9; DQ381 0GC encrypted). Write-ups in Documents/dsg_re/.
        Paddles now send ONE clean edge per pull instead of holding the level:
            on the paddle's rising edge we send the tip bit for one pulse, then
            force it back to 0, and lock the next shift out for
            PADDLE_SHIFT_LOCKOUT_MS (300 ms). This matches how the gearbox really
            accepts a tip (edge-triggered, a one-shot latch that only re-arms on
            release; the OEM gateway log shows ~302 ms minimum between shifts).
            Fixes the two faults: a release bounce used to make a second edge
            (one pull, two gears - worst on downshifts, which the MQB firmware
            doesn't latch) and a quick re-tap used to land in the shift-busy
            window and be dropped (press twice). Holding a paddle now gives
            exactly one shift, like the factory.
        MQB DSG speed over UDS corrected: was a placeholder (DID 0x0101, /100 on
            the OBD2 broadcast 0x7DF). Now DID 0x100A "Vehicle speed from
            transmission" on the DSG's own IDs 0x7E1/0x7E9, read as 16-bit
            little-endian signed, km/h = raw/10. (PQ DSG speed is unchanged -
            TP2.0 KWP 21 01, raw/3 - which the DQ500 firmware confirmed is right.)
            The DID number and raw/10 want one on-car check: the DID table sits
            in a flash region the FRF didn't contain.
        Paddle frame still chosen on the web page (Off / MQB 0x3DD / PQ 0x38A /
            both): a PQ gearbox only reads 0x38A, an MQB one only 0x3DD, so the
            setting must match the box (a wrong one does nothing).

*/