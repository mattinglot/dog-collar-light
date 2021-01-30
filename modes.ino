
/**
 * This is sleep mode - light turns off until button is pressed
 */

bool runMode0( unsigned long cycleTime ) {

  // turn off light
  setLedState(false, false);
  setColor(0,0,0,false);

  if( USE_SLEEP_MODE )
    turnOff(); // go to sleep 
  return true;
}


bool runMode1( unsigned long cycleTime ) {
  unsigned long effectStart = 0;
  const unsigned long ledEffectRate = 700;
  const unsigned long colorShiftRate = 4900;

  // using colors rather than leds to track effectStart var, so not grabbing it here
  effectAlternateLeds( cycleTime, effectStart, colorShiftRate * 50, ledEffectRate );
  //effectAlternateLeds( millis(), 0, millis() + 5000, ledEffectRate );

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    0,0,100, 
    30,0,160
    );
  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    30,0,160, 
    120,0,70
    );

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate,  
    120,0,70,
    90,0,0
    );

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate,  
    90,0,0,
    0,120,0
    );
    
  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate,  
    0,120,0,
    90,150,0
    );  

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate,  
    90,150,0,
    0,0,30
    );

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate,  
    0,0,30,
    0,0,100
    );


  if( cycleTime > effectStart )
    return true;

  return false;
}


bool runMode2( unsigned long cycleTime ) {
  unsigned long effectStart = 0;

  // using colors rather than leds to track effectStart var, so not grabbing it here
  //effectPulseLeds( cycleTime, effectStart, 10000, 250 );

  setLedState(true,true);

  effectStart = effectColorShift( 
      cycleTime, effectStart, 300, 
      0,0,0, 
      120,0,0
      );

  effectStart = effectColorShift( 
      cycleTime, effectStart, 200, 
      120,0,0, 
      180,0,0
      );
      
  effectStart = effectColorShift( 
    cycleTime, effectStart, 200, 
    180,0,0, 
    120,0,0
    );

  effectStart = effectColorShift( 
    cycleTime, effectStart, 300, 
    120,0,0, 
    0,0,0
    );

  effectStart = effectColorShift( 
    cycleTime, effectStart, 400, 
    0,0,0, 
    0,0,0
    );        
    
  if( cycleTime > effectStart )
    return true;

  return false;
}

bool runMode3( unsigned long cycleTime ) {
  return runModeChristmas(cycleTime);
}

bool runModeChristmas( unsigned long cycleTime ) {
  unsigned long effectStart = 0;
  unsigned long blinkDuration = 800;

  // using colors rather than leds to track effectStart var, so not grabbing it here
  //effectPulseLeds( cycleTime, effectStart, 10000, 250 );

  if( cycleTime < blinkDuration )
    setLedState(true,false);
  else
    setLedState(false,true);

  effectStart = effectColorShift( 
      cycleTime, effectStart, blinkDuration / 2, 
      0,30,0, 
      0,200,0
      );

  effectStart = effectColorShift( 
      cycleTime, effectStart, blinkDuration / 2, 
      0,200,0, 
      0,30,0
      );

  effectStart = effectColorShift( 
      cycleTime, effectStart, blinkDuration / 2, 
      30,0,0, 
      200,0,0
      );

  effectStart = effectColorShift( 
      cycleTime, effectStart, blinkDuration / 2, 
      200,0,0, 
      30,0,0
      );

    
  if( cycleTime > effectStart )
    return true;

  return false;
}


/**
 * Checks to see if button has been pressed rapidly 
 */

bool runModeUtilityWake( unsigned long cycleTime ) {
unsigned long currentButtonPress = 0;
static unsigned long buttonPressStart = 0;
static bool inButtonPress = false;
static unsigned int totalPresses = 0;

  

  if( digitalRead(BUTTON_MODE) == HIGH )
  {
    if( buttonPressStart == 0 || inButtonPress == false )
      buttonPressStart = cycleTime;

    inButtonPress = true;
    setLedState(true,true);
    setColor(180,0,0);
  }
 

  if( digitalRead(BUTTON_MODE) == LOW )
  {
    if( inButtonPress ) {
      currentButtonPress = cycleTime - buttonPressStart;
      
      if( currentButtonPress > DEBOUNCE_MILLIS && currentButtonPress < DEBOUNCE_MILLIS_LONG_PRESS )
        totalPresses++;
    }

    setLedState(false,false);
    setColor(0,0,0);

    inButtonPress = false;
  }

  if( totalPresses >= PRESSES_TO_START ) {
    buttonPressStart = 0;
    totalPresses = 0;

    // start cooldown to avoid excess button presses changing modes
    startMode(1);
    coolDownStart = millis();
  }

  // if user hasn't pressed the button enough times fast enough go back to sleep
  if( cycleTime > MILLIS_TO_START ) {
    buttonPressStart = 0;
    totalPresses = 0;
    startForcedSleep();
    startMode(MODE_UTILITY_OFF);
  }

  /*// button has met threshold for being on, so start the first mode
  if( cycleTime > MILLIS_TO_START )
    startMode(1);

  // button got turned off, so go back to sleep
  if( digitalRead(BUTTON_MODE) == LOW )
    startMode(0);*/
    
  return false;
}

bool runModeUtilityTest( unsigned long cycleTime ) {
const unsigned long colorShiftRate = 4900;
unsigned long effectStart = 0;

  setLedState(0,1);

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    0,0,0, 
    0,0,255
    );


  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    0,0,255, 
    0,0,255
    );    

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    0,0,0, 
    0,255,0
    );


  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    0,255,0, 
    0,255,0
    );

  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    0,0,0, 
    255,0,0
    );


  effectStart = effectColorShift( 
    cycleTime, effectStart, colorShiftRate, 
    255,0,0, 
    255,0,0
    );     

  if( cycleTime > effectStart )
    return true;
  
  return false;
}
