/* 
CAN-BUS converter to Digital Output.  Used for MK2/MK3 'analog' clusters in ME7.x and aftermarket conversions and will provide an EML/EPC light.
All outputs are configurable 12v Square Wave with definable max limits based on x RPM / Speed etc.  All outputs are 200mA max(!).  Reverse is MOSFET and 5A max(!).

Main features:
> 12v Positive output for reverse light (5A max!)
> RPM/Speed/EML/EPC outputs (200mA max!)
> DSG Paddles (ground to activate)
> Needle sweep & shift light
> WiFi config.

Forbes-Automotive, 2025
*/

#include "can2cluster.h"

// can2cluster.cpp intentionally left mostly empty to avoid duplicate
// global definitions / timer handlers. main.cpp is the primary translation
// unit for program entry and shared object definitions.

