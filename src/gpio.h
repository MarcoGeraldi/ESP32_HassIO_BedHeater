#ifndef _GPIO_H
#define _GPIO_H

/* -------------------------------------------------------------------------- */
/*                                Include Files                               */
/* -------------------------------------------------------------------------- */
#include <Arduino.h>

/* -------------------------------------------------------------------------- */
/*                                   Macros                                   */
/* -------------------------------------------------------------------------- */
#define BTN_UP_PIN   5
#define BTN_DOWN_PIN 15 
#define BTN_CNTR_PIN 18 
#define SSR_PIN      2

#define numButtons 3

/* -------------------------------------------------------------------------- */
/*                                Enumerations                                */
/* -------------------------------------------------------------------------- */

/* ------------------------------ Button states ----------------------------- */
enum ButtonState
{
    IDLE,
    DEBOUNCE,
    PRESSED,
    SHORT_PRESS,
    SINGLE_PRESS,
    DOUBLE_PRESS,
    LONG_PRESS,
    LONG_RELEASE
};

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
// Constants
extern const int debounceDelay ;         // Debounce delay in milliseconds
extern const int longPressDuration ;    // Duration for long press in milliseconds
extern const int doublePressThreshold ; // Maximum delay between presses for a double press

// Button pins (update according to your setup)
extern int buttonPins[numButtons] ;
extern int buttonPresses[numButtons] ;
extern bool singlePresses[numButtons] ;
extern bool doublePresses[numButtons] ;
extern bool longPresses[numButtons] ;
extern bool longReleases[numButtons] ;

// Variables for button state tracking
extern ButtonState buttonStates[numButtons];
extern unsigned long stateStartTime[numButtons];
extern unsigned long pressStartTime[numButtons];
extern bool lastButtonState[numButtons] ;
extern bool sm_lastBtnState[numButtons] ;

/* -------------------------------------------------------------------------- */
/*                               Data Structures                              */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                                  Typedefs                                  */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
void handleButtonState(int buttonIndex, unsigned long currentTime);
void gpio_init();

#endif
