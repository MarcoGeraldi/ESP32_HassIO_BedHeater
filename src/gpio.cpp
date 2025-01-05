#include "gpio.h"

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
// Constants
const int debounceDelay = 10;         // Debounce delay in milliseconds
const int longPressDuration = 300;    // Duration for long press in milliseconds
const int doublePressThreshold = 20; // Maximum delay between presses for a double press

// Button pins (update according to your setup)
int buttonPins[numButtons] = {BTN_UP_PIN, BTN_DOWN_PIN, BTN_CNTR_PIN};
int buttonPresses[numButtons] = {0};
bool singlePresses[numButtons] = {0};
bool doublePresses[numButtons] = {0};
bool longPresses[numButtons] = {0};
bool longReleases[numButtons] = {0};

// Variables for button state tracking
ButtonState buttonStates[numButtons] = {IDLE};
unsigned long stateStartTime[numButtons] = {0};
unsigned long pressStartTime[numButtons] = {0};
bool lastButtonState[numButtons] = {false};
bool sm_lastBtnState[numButtons] = {false};


void handleButtonState(int buttonIndex, unsigned long currentTime)
{
    bool currentButtonState = !digitalRead(buttonPins[buttonIndex]); // Active LOW button
    // bool stateChanged = (currentButtonState != lastButtonState[buttonIndex]);
    bool stateChanged = (buttonStates[buttonIndex] != sm_lastBtnState[buttonIndex]);

    switch (buttonStates[buttonIndex])
    {
    case IDLE:
        /* ------------------------- Reset number of presses ------------------------ */
        buttonPresses[buttonIndex] = 0;

        if (currentButtonState)
        { // Button pressed
            buttonStates[buttonIndex] = DEBOUNCE;
            stateStartTime[buttonIndex] = currentTime;
        }
        break;

    case DEBOUNCE:
        if ((currentTime - stateStartTime[buttonIndex]) > debounceDelay)
        {
            if (currentButtonState)
            { // Confirmed press
                buttonStates[buttonIndex] = PRESSED;
                pressStartTime[buttonIndex] = currentTime;
            }
            else
            { // False press
                buttonStates[buttonIndex] = IDLE;
            }
        }
        break;

    case PRESSED:

        if (!currentButtonState)
        {
            // Button released
            buttonPresses[buttonIndex]++;
            buttonStates[buttonIndex] = SHORT_PRESS;
            stateStartTime[buttonIndex] = currentTime;
        }
        else if ((currentTime - pressStartTime[buttonIndex]) > longPressDuration)
        {
            // Long press detected
            longPresses[buttonIndex] = true;
            buttonStates[buttonIndex] = LONG_PRESS;
        }
        break;
    case SHORT_PRESS:
        if (currentButtonState && (currentTime - stateStartTime[buttonIndex] > debounceDelay))
        {
            // Second press detected after debounce
            buttonStates[buttonIndex] = DEBOUNCE;
            stateStartTime[buttonIndex] = currentTime; // Reset debounce timer
        }
        else if ((currentTime - stateStartTime[buttonIndex]) > doublePressThreshold)
        {
            // Timeout for second press, register as single press
            if (buttonPresses[buttonIndex] == 1)
            {
                buttonStates[buttonIndex] = SINGLE_PRESS;
            }
            else if (buttonPresses[buttonIndex] > 1)
            {
                buttonStates[buttonIndex] = DOUBLE_PRESS;
            }
        }
        break;

    case LONG_PRESS:

        if (!currentButtonState)
        { // Button released after long press
            buttonStates[buttonIndex] = LONG_RELEASE;
        }
        break;

    case LONG_RELEASE:
        longReleases[buttonIndex] = true;
        buttonStates[buttonIndex] = IDLE;
        break;

    case SINGLE_PRESS:
        singlePresses[buttonIndex] = true;
        buttonStates[buttonIndex] = IDLE;
        break;

    case DOUBLE_PRESS:
        doublePresses[buttonIndex] = true;
        buttonStates[buttonIndex] = IDLE;
        break;
    }

    sm_lastBtnState[buttonIndex] = buttonStates[buttonIndex];
    lastButtonState[buttonIndex] = currentButtonState;
}

void gpio_init()
{
    /* --------------------- Set button pins as input pullup -------------------- */
    for (int i = 0; i < numButtons; i++)
    {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }

    /* -------------------- Initialize SSR control output pin ------------------- */
    pinMode(SSR_PIN, OUTPUT);
}