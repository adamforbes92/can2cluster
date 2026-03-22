#include "can2cluster_buttons.h"

static unsigned long padUpPressTime = 0;
static unsigned long padDownPressTime = 0;
#define PADDLE_FEEDBACK_DURATION 2000  // milliseconds

void updateButtons(void) {
  btnPadUp.tick();
  btnPadDown.tick();
}

void padUpFunc(void) {
  boolPadUp = true;
  padUpTxPending = true;
  padUpPressTime = millis();
}

void padDownFunc(void) {
  boolPadDown = true;
  padDownTxPending = true;
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