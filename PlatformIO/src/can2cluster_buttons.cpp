#include "can2cluster_buttons.h"

// "static" here = only this file uses these two
static unsigned long padUpPressTime = 0;   // when paddle + was last pulled (web page feedback)
static unsigned long padDownPressTime = 0; // when paddle - was last pulled (web page feedback)
#define PADDLE_FEEDBACK_DURATION 2000  // milliseconds

// Read both paddle switches. Called every 10 ms by broadcastPaddles() in
// can2cluster_can.cpp - the only place that reads them, so what's read and
// what goes on the CAN bus always agree.
// Paddles are plain GPIO inputs on both boards (pins set in _defs.h).
void updateButtons(void) {
  btnPadUp.tick();   // OneButton reads the pin and debounces it
  btnPadDown.tick();

  // debouncedValue() is 1 while the paddle is held and 0 once it's let go.
  // (OneButton's "click" isn't used any more: it only fired when the paddle
  // was let go, and never at all if it was held for more than 0.8 s.)
  bool upHeldNow = (btnPadUp.debouncedValue() == 1);
  bool downHeldNow = (btnPadDown.debouncedValue() == 1);

  // A fresh pull lights the web page feedback for a couple of seconds
  if (upHeldNow && !isPaddleUpHeld) {
    padUpFunc();
  }
  if (downHeldNow && !isPaddleDownHeld) {
    padDownFunc();
  }

  isPaddleUpHeld = upHeldNow;
  isPaddleDownHeld = downHeldNow;
}

// Paddle + has just been pulled: show it on the web page
void padUpFunc(void) {
  boolPadUp = true;
  padUpPressTime = millis();
}

// Paddle - has just been pulled: show it on the web page
void padDownFunc(void) {
  boolPadDown = true;
  padDownPressTime = millis();
}

// Called periodically to auto-reset paddle feedback after brief display
void updatePaddleFeedback(void) {
  unsigned long currentTime = millis();

  if (boolPadUp && (currentTime - padUpPressTime > PADDLE_FEEDBACK_DURATION)) {
    boolPadUp = false;
  }

  if (boolPadDown && (currentTime - padDownPressTime > PADDLE_FEEDBACK_DURATION)) {
    boolPadDown = false;
  }
}
