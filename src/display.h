#ifndef _DISPLAY_H
#define _DISPLAY_H

/* -------------------------------------------------------------------------- */
/*                                Include Files                               */
/* -------------------------------------------------------------------------- */
#include <Wire.h>
#include <Adafruit_SSD1306.h>
 
#include "error.h"

/* -------------------------------------------------------------------------- */
/*                                   Macros                                   */
/* -------------------------------------------------------------------------- */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define SET_LABEL_TEXT_SIZE 1      // Text size for "SET:"
#define TEMP_TEXT_SIZE 4           // Text size for the temperature reading
#define SETPOINT_TEXT_SIZE 2       // Text size for the setpoint value

#define SLEEP_TIMEOUT 5            //seconds

/* -------------------------------------------------------------------------- */
/*                                Enumerations                                */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

// Simulated variables
extern bool relayOn;       // Relay state (true = on, false = off)

extern unsigned long displayTimeout;

/* -------------------------------------------------------------------------- */
/*                               Data Structures                              */
/* -------------------------------------------------------------------------- */
extern Adafruit_SSD1306 display;
/* -------------------------------------------------------------------------- */
/*                                  Typedefs                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
error_t displayInit();
void updateDisplay(double _temperature, int _setpoint, bool _output, error_t _errorCode);
void sleepDisplay();
void wakeDisplay();

#endif