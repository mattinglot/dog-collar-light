/**
 * Sets the color of the RGB leds and their on state
 */

void setColor( unsigned int red, unsigned int green, unsigned int blue, bool checkLevels = true ) {

  if( checkLevels ) {
    // don't surpass max brightness levels so we don't blow battery
    if( red > MAX_BRIGHTNESS )
      red = MAX_BRIGHTNESS;
  
    if( green > MAX_BRIGHTNESS )
      green = MAX_BRIGHTNESS;
  
    if( blue > MAX_BRIGHTNESS )
      blue = MAX_BRIGHTNESS;

    // MIN_BRIGHTNESS allows us to filter out an led being turned on at minimum frequency
    // due to rounding errors, which can cause very odd behavior otherwise especially with
    // the low default resolution of ATTiny PWM.
    if( red < MIN_BRIGHTNESS )
      red = 0;

    if( green < MIN_BRIGHTNESS )
      green = 0;

    if( blue < MIN_BRIGHTNESS )
      blue = 0;
  
    // calculate the units of "output" that we are sending to the leds
    int output = green + red + blue;
  
    if( ledStates[0] && ledStates[1] )
      output *= 2;

    // if we are exceed the LED output the dim the LEDs below the max
    if( output > MAX_LED_OUTPUT ) {
      red = red * MAX_LED_OUTPUT / output;
      green = green  * MAX_LED_OUTPUT / output;
      blue = blue  * MAX_LED_OUTPUT / output;
      //red = green = blue = MAX_LED_OUTPUT / 3;
    }
  }

  analogWrite( LED_RED, pgm_read_byte(&gamma8[red]) );
  analogWrite( LED_GREEN, pgm_read_byte(&gamma8[green]) );
  analogWrite( LED_BLUE, pgm_read_byte(&gamma8[blue]) );

  currentColors[RED] = red;
  currentColors[GREEN] = green;
  currentColors[BLUE] = blue;
}


/**
 * Sets the on/off state of leds
 */

void setLedState( bool led1On, bool led2On ) {

  if( ledStates[0] != led1On )
    digitalWrite( LED_SWITCH_1, led1On ? HIGH : LOW );

  if( ledStates[1] != led2On )
    digitalWrite( LED_SWITCH_2, led2On ? HIGH : LOW );

  ledStates[0] = led1On;
  ledStates[1] = led2On;
}


/**
 * Shifts colors from their current state to the given target colors, smoothly over
 * the given duration. Effects could be refactored into a base Effect class that better
 * handles the bookkeeping associated with keeping time.
 */

unsigned long effectColorShift( unsigned long cycleTime, unsigned long effectStart, unsigned long duration, unsigned int startRed, unsigned int startGreen, unsigned int startBlue, unsigned int targetRed, unsigned int targetGreen, unsigned int targetBlue ) {
  unsigned long red = 0;
  unsigned long green =0;
  unsigned long blue =0;    
  int effectTime = cycleTime - effectStart;


  
  if( cycleTime > effectStart && cycleTime < (effectStart + duration) ) {

    // keeping everything unsigned long is very important here, otherwise you spend
    // an afternoon trying to track down super weird behavior because targetxxx and startxxx
    // are ints.

    if( targetRed > startRed ){
      red = ((unsigned long) targetRed- (unsigned long) startRed) * effectTime / duration;
      red += startRed;
    }
    else {
      red = ((unsigned long) startRed- (unsigned long) targetRed) * (duration - effectTime) / duration;
      red += targetRed;
    }
    
    if( targetGreen > startGreen ) {
      green = ((unsigned long) targetGreen- (unsigned long) startGreen) * effectTime / duration;
      green += startGreen;
    }
    else {
      green = ((unsigned long) startGreen- (unsigned long) targetGreen) * (duration - effectTime) / duration;
      green += targetGreen;
    }

    if( targetBlue > startBlue ) {
      blue = ((unsigned long) targetBlue - (unsigned long) startBlue) * effectTime / duration;
      blue += startBlue;
    }
    else {
      blue = ((unsigned long) startBlue - (unsigned long) targetBlue) * (duration - effectTime) / duration;
      blue += targetBlue;
    }
       
    setColor( red, green, blue);
  }

  return effectStart + duration;
}


/**
 * Alternates the leds according to set frequency (meaning switch every _____ milliseconds)
 */

unsigned long effectAlternateLeds( unsigned long cycleTime, unsigned long effectStart, unsigned long duration, unsigned long frequency ) {

  bool led1On;
  bool led2On;
  int effectTime = cycleTime - effectStart;

   if( cycleTime > effectStart && cycleTime < (effectStart + duration) ) {

    if( (effectTime / frequency) % 2 ) {
      led1On = true;
      led2On = false;
    } else {
      led1On = false;
      led2On = true;
    }

    setLedState( led1On, led2On );

  }

  return effectStart + duration;
}

/**
 * Pulses both LEDs on/off at the same time 
 */

unsigned long effectPulseLeds( unsigned long cycleTime, unsigned long effectStart, unsigned long duration, unsigned long frequency ) {

  bool led1On;
  bool led2On;
  int effectTime = cycleTime - effectStart;

   if( cycleTime > effectStart && cycleTime < (effectStart + duration) ) {

    if( (effectTime / frequency) % 2 ) {
      led1On = true;
      led2On = true;
    } else {
      led1On = false;
      led2On = false;
    }

    setLedState( led1On, led2On );

  }

  return effectStart + duration;
}
