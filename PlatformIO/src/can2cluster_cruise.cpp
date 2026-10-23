#include "can2cluster_cruise.h"

// "static" here = only this file uses these
static bool lastResumeReading = false; // RESUME pin as read on the last pass (before the debounce)
static uint32_t resumeChangedMs = 0;   // millis() when that reading last changed
static bool wasMainOn = false;         // main switch on the last pass
static uint32_t cancelStartMs = 0;     // millis() when the main switch was switched off (0 = not cancelling)

// Read the MK4 cruise stalk and work out GRA_Neu bytes 1 and 2.
// The main switch (pin 10) and SET (pin 11) come in already debounced - they're
// the paddle inputs, read by updateButtons(). RESUME (pin 12, the hall speed
// input) is read and debounced here. Behaviour copied from the customer's MK4
// build (v4.01.2), which they checked on their car.
void updateCruiseStalk(bool isMainOnNow, bool isSetPressedNow, uint32_t now)
{
  // --- RESUME on pin 12: grounded (LOW) = pressed ---
  bool resumeReading = false;
  if (digitalRead(pinHallSensor) == LOW)
  {
    resumeReading = true;
  }

  if (resumeReading != lastResumeReading)
  {
    lastResumeReading = resumeReading; // it changed: start timing again
    resumeChangedMs = now;
  }
  else if ((now - resumeChangedMs) >= CRUISE_RESUME_DEBOUNCE_MS)
  {
    isCruiseResumePressed = resumeReading; // steady for long enough: take it
  }

  isCruiseMainOn = isMainOnNow;         // for the web page
  isCruiseSetPressed = isSetPressedNow; // for the web page

  // --- Main switch just switched off: send "cancel" for a moment ---
  if (wasMainOn && !isMainOnNow)
  {
    cancelStartMs = now;
    if (cancelStartMs == 0)
    {
      cancelStartMs = 1; // 0 means "not cancelling", so never store it
    }
  }
  wasMainOn = isMainOnNow;

  bool isCancelling = false;
  if (cancelStartMs != 0 && (now - cancelStartMs) < CRUISE_CANCEL_MS)
  {
    isCancelling = true;
  }
  else
  {
    cancelStartMs = 0; // cancel finished
  }

  // --- GRA_Neu byte 1: main switch and cancel ---
  uint8_t buttons = 0;
  if (isMainOnNow)
  {
    buttons = buttons | GRA_MAIN_SWITCH; // bit 0: main switch on
  }
  if (isCancelling)
  {
    buttons = buttons | GRA_CANCEL; // bit 1: cancel
  }

  // --- GRA_Neu byte 2 bits 0-1: SET and RESUME, only while the main switch is on ---
  uint8_t setResume = 0;
  if (isMainOnNow && isSetPressedNow)
  {
    setResume = setResume | GRA_SET; // bit 0: SET
  }
  if (isMainOnNow && isCruiseResumePressed)
  {
    setResume = setResume | GRA_RESUME; // bit 1: RESUME
  }

  cruiseButtonsByte = buttons;
  cruiseSetResumeBits = setResume;
}
