#ifndef _CONFIG_H 
#define _CONFIG_H 

#define LED_RED 6
#define LED_GREEN 7
#define LED_BLUE 8
#define LED_SWITCH_1 2 //0
#define LED_SWITCH_2 1
#define BUTTON_MODE 0 // PA0/PCINT0

// lowest "on" value for LED. Do NOT use to turn color off
#define MIN_BRIGHTNESS 2

// highest "on" value for LED, aka the brightest setting we allow on LED
#define MAX_BRIGHTNESS 255

// maximum "output" across all LEDs. If multiple LEDs are on such that this 
// output is exceeded then the LEDs are throttled
#define MAX_LED_OUTPUT 440

// number of distinct brightness levels available to us
#define STEPS (MAX_BRIGHTNESS - MIN_BRIGHTNESS)

#define RED 0
#define GREEN 1
#define BLUE 2

#define MILLIS_TO_START 1500 // how many milliseconds user has to press button PRESSES_TO_START times to turn on
#define PRESSES_TO_START 3 // how many times button has to be rapidly pressed on/off to start

#define START_COOLDOWN_MILLIS 500 // milliseconds button presses are ignored entirely after start to avoid accidentally switching modes

// uncomment to enable the ability to press button rapidly to turn off similar to
// how the start function works
//#define RAPID_PRESS_OFF
#define MILLIS_TO_OFF 1000   // how many milliseconds on button must be pressed to turn off light
#define PRESSES_TO_OFF 4 // how many times button has to be rapidly pressed on/off to turn off

#define DEBOUNCE_MILLIS 50 // how many milliseconds a button must be debounced for
#define DEBOUNCE_MILLIS_LONG_PRESS 500 // how many milliseconds is considered a long press

#define AUTO_OFF_MILLIS (60*60*1.5*1000) // turn off automatically after 1.5 hours

#define USE_SLEEP_MODE true // if false does not use sleep when powering done, for debug only!

#endif // _CONFIG_H
