#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h> //Needed to enable/disable watch dog timer
#include <EEPROM.h>

#include "config.h"
#include "colorlib.h"
#include "modes.h"

// where various data is stored in EEPROM
#define EEPROM_ADDR_ACTIVE_MODE 0x02 // last active mode

typedef struct ColorShift {
  unsigned int cycleTime; // how long should this shift take
  unsigned char colors[3];
};


int verifyLightLevel = 10; // the light level to set verify route to in the beginning
volatile int interruptBrightness = 10;

volatile unsigned long modeStartTime = 0; // the millis() time that this mode started at
volatile byte activeMode = 0; // the currently active light show mode.
volatile bool isSleeping = false; // whether we are currently in sleep mode

volatile bool inWatchdog = false; // set to true when entering watchdog, set to false when exiting

// turning the light off requires a number of button presses to occur in a short
// interval. We handle this by checking if the button has been pressed (actually if
// mode has been changed) in rapid succession.
unsigned long offButtonPressStart = 0;
unsigned int offButtonPressCount = 0;

// track whether we are in cooldown mode on start to ensure button presses are ignored
unsigned long coolDownStart = 0;

// when was the light last turned on
unsigned long lastOn = 0;

// Power saving measure to force the light to go to sleep if processor stays on and NOT
// in an "on" state for too long. Such as someone holding down the button when the product
// is off.
volatile bool forcedSleep = false;

// used to indicate that the current state should be saved to eeprom
// this is used a part of system to delay saving the active mode to eeprom
// by 1s to avoid rapid switching between modes causing wasteful eeprom writes
volatile bool eepromSaveRequired = false;

int currentColors[3] = {0,0,0}; 
int ledStates[2] = {0,0};

void setup() {
  
  // put your setup code here, to run once:
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_SWITCH_1, OUTPUT);
  pinMode(LED_SWITCH_2, OUTPUT);
  pinMode(BUTTON_MODE, INPUT);

  // CONFIGURE TIMERS FOR PWM
  
  /*
   * WGM10, WGM12: Fast PWM, 8-bit TOP=255
   * CS10: No prescaler
   * COM1A1: Pin 6 to LOW on match with OCR1A and to HIGH on TOP
   */
  //TCCR1A = _BV(COM1A1) | _BV(WGM10);
  //TCCR1B = _BV(CS10) | _BV(WGM12);
  /*
   * 50% duty cycle
   * 4 kHz with 1MHz CPU clock
   */
  //OCR1A = 127;

  // CONFIGURE INTERRUPTS
  // turn on interrupts.
  //PCMSK0 |= 0b00000100;     // PCINT2  (PA2/pin 11)

  MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on any change
  //MCUCR &= (_BV(ISC01) | _BV(ISC00)); // trigger on rising edge
  
  
  
  //MCUCR |= _BV(ISC00); // set INT0 to trigger on rising edge
  PCMSK0 |= 0b00000001;     // PCINT0  (PA0/pin 13)
  //GIFR   = 0b00000000;     // clear any outstanding interrupts
  GIMSK  |= 0b00010000;     // enable external interrupt
  //GIMSK |= _BV(INT0);
  //GIMSK |= _BV(PCINT0);
  sei();                 // enables interrupts


  startMode(getLastSavedMode());
  //startMode(MODE_UTILITY_OFF);
  //startMode(MODE_UTILITY_TEST);
}

void loop() {

  // verifies the function of the leds
  //verifyLeds();
  //verifyLedsFull(); 

   // run the active mode
   runActiveMode();

   // don't process button presses if we are cooling down from start up
   if( coolDownStart != 0 ) {
    if( millis() - coolDownStart > START_COOLDOWN_MILLIS)
      coolDownStart = 0;
   }
   else {
    // check the state of the button to see if anything needs to be done
    checkButtonState();
   }

   if( (millis() - lastOn) > AUTO_OFF_MILLIS ) { 
    startMode(MODE_UTILITY_OFF);
   }
}


/**
 * Returns the last mode that was on (saved via EEPROM)
 * or MODE_UTILITY_OFF if no valid mode found.
 */

byte getLastSavedMode() {
  byte mode;
  EEPROM.get(EEPROM_ADDR_ACTIVE_MODE, mode);
  return mode;
}


/**
 * Saves the current state of the light to eeprom. Used so that if 
 * dog manages to momentary cause light to lose power due to loose
 * battery connection, light will resume previous mode rather than
 * turn off.
 */

void saveStateToEeprom() {
  // if we are off or in a regular (not utility) mode then save the current mode
  // don't save utility modes as we don't want to start up in utility mode and just
  // wastes eeprom (which has limited writes)
  if( activeMode <= LAST_MODE )
    EEPROM.update(EEPROM_ADDR_ACTIVE_MODE, activeMode);
}


/**
 * Turns on a given light mode and resets it to start from scratch
 */

void startMode( int mode ) {

  // if turning off then immediately save that state to eeprom
  // else flag it for saving
  if( mode == MODE_UTILITY_OFF )
    saveStateToEeprom();
  else if( mode != activeMode)
    eepromSaveRequired = true;
    
  activeMode = mode;
  modeStartTime = millis();
}


unsigned long funGradient( unsigned long cycleTime, unsigned long effectStart, unsigned long duration ) {
  bool led1On = false;
  bool led2On = false;

  int effectTime = cycleTime - effectStart;
  int red;
  int green;
  int blue;
  int redSteps;
  int greenSteps;
  int blueSteps;

  ColorShift colorstest[3] = {
    { 2500, 0, 10, 100 },
    { 2500, 0, 10, 100 },
  };

  unsigned char colors[3][3] = {
    {0,10,100},{100,10,0},{50,0,50}
    };

  if( cycleTime > effectStart && cycleTime < (effectStart + duration) ) {

    int colorIndex = (int) (floor(effectTime / (duration / 3)));

    if( colorIndex < 0 )
      colorIndex = 0;

    if( colorIndex > 2 )
      colorIndex = 2;
    
    // want Red to go from Min to Max over duration
    if( colors[colorIndex][RED] > 0 ) {
      redSteps = duration / colors[colorIndex][0];
      red = effectTime / redSteps + MIN_BRIGHTNESS;
    }
    else
      red = 0;

    if( colors[colorIndex][GREEN] > 0 ) {
      greenSteps = duration / colors[colorIndex][1];
      green = effectTime / greenSteps + MIN_BRIGHTNESS;
    }
    else
      green = 0;

    if( colors[colorIndex][BLUE] > 0 ) {
      blueSteps = duration / colors[colorIndex][2];
      blue = effectTime / blueSteps + MIN_BRIGHTNESS;
    }
    else
      blue = 0;

    if( (effectTime / 1000) % 2 ) {
      led1On = true;
      led2On = false;
    } else {
      led1On = false;
      led2On = true;
    }

    setLedState( led1On, led2On );
    setColor( red, green, blue, false );
  }

  return effectStart + duration;
}


void turnOff() {
  ADCSRA &= ~_BV(ADEN);      // ADC off
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // enable sleep mode
  sleep_enable(); 
  sei();              

  isSleeping = true;

  // sleep cpu until an interrupt occurs
  sleep_cpu();    

  // wakeup from sleep tasks.
  sleepWake();
}


/**
 * Wakeup from sleep.
 */

void sleepWake() {

  cli();    
  sleep_disable();                        // Clear SE bit
  ADCSRA |= _BV(ADEN);                    // ADC on
  sei();
  lastOn = millis();
  isSleeping = false;
}

void sleepIdle( int length ) {

  if (length > 9) 
    length = 9; //Limit incoming amount to legal settings

  byte bb = length & 7;
  
  if (length > 7) 
    bb |= (1 << 5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1 << WDRF); //Clear the watch dog reset
  WDTCSR |= (1 << WDCE) | (1 << WDE); //Set WD_change enable, set WD enable
  WDTCSR = bb; //Set new watchdog timeout value
  WDTCSR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
  inWatchdog = true;

  isSleeping = true;
  // use ide sleep mode which still allows PWM to function
  set_sleep_mode(SLEEP_MODE_IDLE);

  // attiny frequently exits idle mode to update its own timer
  // so keep going back to sleep until inWatchdog has been set to false by
  // watchdog interrupt
  while( inWatchdog ) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
    
  isSleeping = false;
  ADCSRA |= _BV(ADEN);
}


void runActiveMode() {
  // convert mode time to how much time has elapsed in the current cycle
  unsigned long cycleTime = millis() - modeStartTime;
  bool restartMode = false;

  switch( activeMode )
  {
    case 0:
      restartMode = runMode0(cycleTime);
      break;
    
    case 1:
      restartMode = runMode1(cycleTime);
      break;

    case 2:
      restartMode = runMode2(cycleTime);
      break;

    case 3:
      restartMode = runMode3(cycleTime);
      break;

    case MODE_UTILITY_WAKE:
      restartMode = runModeUtilityWake(cycleTime);
      break;

    case MODE_UTILITY_TEST:
      restartMode = runModeUtilityTest(cycleTime);
      break;      

    /// if in unrecognized mode then switch off
    default:
      startMode(MODE_UTILITY_OFF);
      restartMode = true;
  }

  // To preserve eeprom write cycles, the current mode is only saved if it has been
  // active for at least a second or one full mode cycle, whichever comes first.
  if( eepromSaveRequired && (restartMode || cycleTime > 1000) ) {
    saveStateToEeprom();
    eepromSaveRequired = false;
  }

  if( restartMode )
    startMode(activeMode);
}


/**
 * Figures out what mode should be next based on current mode. Only
 * runs through active modes
 */

void startNextMode()
{
  activeMode++;

  if( activeMode > MODE_COUNT )
    activeMode = 1; // gone through all modes so turn off
  
  startMode(activeMode);
}


/**
 * Interrupt handler for watchdog interrupt. 
 * Don't remove or mcu will reset after watchdog ends.
 */

ISR(WDT_vect) {
  inWatchdog = false;
}

/**
 * Interrupt handler for mode button which gets called everytime the 
 * button state changes.
 */

ISR(PCINT0_vect) {

   
  // if we are here, then state of the button has changed so call checkButtonState
  // this interrupt handler is mostly needed for when the light is in sleep mode.
  // Otherwise the checkButtonState gets called during every cycle of the main 
  // loop and we don't continue to call it here (to avoid the interrupt handler calling
  // checkButtonState() while we are already in checkButtonState).
  if( isSleeping )
    checkButtonState(); 
}


/**
 * Returns true if sleep has been forced. Will keep returning true until endForcedSleep
 * is called
 */

bool isForcedSleep()
{
  if( forcedSleep )
  {
    return true;
  }

  return false;
}


/**
 * Starts a mandatory forced sleep to ensure that device goes to 
 * sleep on the next button check no matter what is going on, and if
 * button is pressed down then that press is ignored.
 */

void startForcedSleep()
{
  forcedSleep = true;
}


/**
 * Ends forced sleep mode.
 */

void endForcedSleep()
{
  forcedSleep = false;
}


/**
 * Monitors the state of the on/off button and acts accordingly.
 * To work properly, this function is called everytime the processor
 * loops as well as by the hardware interrupt handler when button state 
 * changes.
 */

void checkButtonState()
{
static unsigned long lastInterruptTime = 0;
  static bool doneInterrupt = false;
  static bool ignoreButtonHigh = false; // if set to true, ignore button high because it's being checked by wakeup
  unsigned long interruptTime = millis();
  static bool inButtonPress = false; // this gets set to true when button position is high
  static unsigned long lastButtonPressStart = 0; // last time button state changed to high
  unsigned long buttonPressDuration = 0; // how long button has been pressed
 
  if( isForcedSleep() )
  {
    if( !ignoreButtonHigh && digitalRead(BUTTON_MODE) == HIGH )
      ignoreButtonHigh = true;

    endForcedSleep();
    startMode(MODE_UTILITY_OFF);
    return;
  }

  // if in the check for wakeup mode then don't do any further processing here
  if( activeMode == MODE_UTILITY_WAKE )
    return;

  // if we are in sleep mode then we can't debounce button in the interrupt since
  // attiny won't increment millis. Instead we wake up and start a special mode that 
  // waits for the power button to be pressed long enough and starts regular mode, or 
  // goes back to sleep. 
  if( !ignoreButtonHigh && isSleeping && (digitalRead(BUTTON_MODE) == HIGH) ) {
    //startNextMode();

    // set this flag to high so we know that the button press is as a result of 
    // waiting for light to turn on. This keeps us from accidentally moving modes
    // if user holds button down too long
    //ignoreButtonHigh = true;
    startMode(MODE_UTILITY_WAKE);
    return;
  }

  // if button goes low we can reset the ignoreButtonHigh flag
  // this also means the interrupt can end here because we've woken up
  // and mode 1 was started by MODE_UTILITY_WAKE
  if( ignoreButtonHigh && digitalRead(BUTTON_MODE) == LOW ) {
    ignoreButtonHigh = false;
    return;
  }

  // if we got to this point and we're ignoring button high then 
  // just return from the handler so that the button press is not
  // processed further
  if( ignoreButtonHigh )
    return;

  // if in sleep mode then don't process further button logic
  if( isSleeping )
    return;

  // NORMAL (NOT ASLEEP) INTERRUPT HANDLER STARTS HERE
  
  // if inside a button press, see how long button has been held down for
  if( inButtonPress )
    buttonPressDuration = millis() - lastButtonPressStart;

  bool doneProcessing = false;

  // check if we've been in button press long enough to turn off light
  // this check happens on button pressed down
  if( !doneProcessing & inButtonPress && digitalRead(BUTTON_MODE) == HIGH ) {

    // check if button was held past threshold to turn off
    if( buttonPressDuration > MILLIS_TO_OFF ) {
      // don't want user continuing to hold button down to turn light
      // back on so ignore high state until button goes low again
      ignoreButtonHigh = true;  
      doneProcessing = true;
      startMode(MODE_UTILITY_OFF);
    }
  }

  // changing modes happens on button release to not conflict with 
  // turn off intent.
  if( !doneProcessing && inButtonPress && digitalRead(BUTTON_MODE) == LOW ) {

    // check if button was held past the debounce minimum
    if( buttonPressDuration > DEBOUNCE_MILLIS ) {

      startNextMode();

      #ifdef RAPID_PRESS_OFF
        
        // check if we are turning off the light
        if( (millis() - offButtonPressStart) < MILLIS_TO_OFF ) {
          // we are still within the MILLIS_TO_OFF range so increment the off button press count
          offButtonPressCount++;
        }
        else {
          // reset off button counter
          offButtonPressCount = 1;
          offButtonPressStart = millis();
        }
  
        if( offButtonPressCount >= PRESSES_TO_OFF ) {
          offButtonPressCount = 0;
          offButtonPressStart = millis();
          startMode(MODE_UTILITY_OFF);
        }
      #endif
      
      doneProcessing = true;
    }
  }

  // adjust state variables to track whether we are in a button press or not
  if( !ignoreButtonHigh && digitalRead(BUTTON_MODE) == HIGH ) {
    if( inButtonPress == false )
      lastButtonPressStart = millis();
      
    inButtonPress = true;
  }
  else
    inButtonPress = false;  
}


/**
 * Verification routine for verifying full brightness of LEDs
 * Used in development to calibrate LED resistor values against
 * real world performance of coin cell batteries.
 */

void verifyLedsFull() {

  int lightLevel = MAX_BRIGHTNESS;
  static int color = RED;

  switch( color ) {
    case RED:
     setLedState(true,false);
     setColor( lightLevel, 0, 0, false );
     break;

    case GREEN:
     setLedState(true,false);
     setColor( 0, lightLevel, 0, false );
     break;
     
    case BLUE:
     setLedState(true,false);
     setColor( 0, 0, lightLevel, false );
     break;
    
  }

  // change which color is active on button press
  if( digitalRead(BUTTON_MODE) == HIGH ) {
    color++;

    if( color > BLUE )
      color = RED;
  }
}


/**
 * Verifies the function of the LEDs by turning each one on
 * individually and holding that value for 5 seconds.
 */

void verifyLeds() {

  int lightLevel = 10;


  setLedState(true,true);
  setColor( lightLevel, 0, 0, false );
  delay(5000);
  setColor( 0, lightLevel, 0, false );
  delay(5000);
  setColor( 0, 0, lightLevel, false );
  delay(5000);

  setLedState(true,false);
  setColor( lightLevel, 0, 0 );
  delay(3000);
  setColor( 0, lightLevel, 0 );
  delay(3000);
  setColor( 0, 0, lightLevel );
  delay(3000);

  setLedState(false,true);
  setColor( lightLevel, 0, 0 );
  delay(3000);
  setColor( 0, lightLevel, 0 );
  delay(3000);
  setColor( 0, 0, lightLevel );
  delay(3000);

  setLedState(true,true);
  setColor( lightLevel, lightLevel, 0, false );
  delay(3000);
  setColor( 0, lightLevel, lightLevel, false );
  delay(3000);
  setColor( lightLevel, 0, lightLevel, false );
  delay(3000);
  setColor( lightLevel, lightLevel, lightLevel, false );
  delay(3000);
}
