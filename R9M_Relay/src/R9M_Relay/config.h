#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "04"

// Turn off all sounds;
//#define SOUND_OFF

// Beep when each FailSafe packet is sent to PXX; 
//#define BEEP_FAILSAFE

// Enable to display MIN/MAX values in menu;
#define RC_MIN_MAX

// Enable to support RELAY mode;
#define RELAY

#define RELAY_CHANNEL    CH7  // Channel to switch between PXX and PPM control; Allowed only channels CH5..CH8;

#define GPS_MODE_CHANNEL CH8  // Set it to channel to enable GPS HOLD mode; Allowed only channels CH5..CH8;
#define GPS_HOLD_VALUE   1600 // Set it to enable GPS HOLD in GPS_MODE_CHANNEL on both relay and mission drone;

#define ACTIVE_PXX_MIN   950
#define ACTIVE_PXX_MAX   1350
#define ACTIVE_CPPM_MIN  1650
#define ACTIVE_CPPM_MAX  2050

#endif
