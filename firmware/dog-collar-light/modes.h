#ifndef _MODES_H 
#define _MODES_H 


// how many modes are available not counting 0/off
#define MODE_COUNT 3

// id of the last regular mode (not utility modes)
#define LAST_MODE MODE_COUNT

// if in this mode, we were sleeping and got woke up, mode deterimines whether key
// press is sufficient to wake up.
#define MODE_UTILITY_WAKE 10

// entry into generic test mode
#define MODE_UTILITY_TEST 11

// turns off the light. Must be 0 for logic to work correctly
#define MODE_UTILITY_OFF 0

bool runMode0( unsigned long cycleTime );
bool runMode1( unsigned long cycleTime );
bool runMode2( unsigned long cycleTime );
bool runMode3( unsigned long cycleTime );

// named modes, to make it easier to later swap out different modes
bool runModeChristmas( unsigned long cycleTime );

// utility modes cannot be accessed normally via button press but other events can 
// trigger them
bool runModeUtilityWake( unsigned long cycleTime );
bool runModeUtilityTest( unsigned long cycleTime );

#endif // _MODES_H
