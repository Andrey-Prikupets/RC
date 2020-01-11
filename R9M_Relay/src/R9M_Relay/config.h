#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "07"

// Show UI in OLED and support buttons;
// If not defined, serial commands are the only way to configure;
//#define OLED

#define CLI_BAUD  115200

// Turn off all sounds;
//#define SOUND_OFF

// Beep when each FailSafe packet is sent to PXX; 
//#define BEEP_FAILSAFE

// Enable to display MIN/MAX values in menu;
#define RC_MIN_MAX

// Emit CPPM in non-RELAY mode;
//#define EMIT_CPPM

// Enable to support RELAY mode;
#define RELAY

#ifdef OLED
#define RELAY_ENABLED    true // Relay mode is enabled by default if no CLI is available;
#else
#define RELAY_ENABLED    false // Relay mode is enabled by default if CLI is available;
#endif
#define RELAY_CHANNEL    CH7  // Channel to switch between PXX and PPM control; Allowed only channels CH5..CH8;

#define GPS_MODE_CHANNEL CH8  // Set it to channel to enable GPS HOLD mode; Allowed only channels CH5..CH8;
#define GPS_HOLD_VALUE   1600 // Set it to enable GPS HOLD in GPS_MODE_CHANNEL on both relay and mission drone;

// RELAY_CHANNEL signal boundaries to enable PXX or CPPM control;
#define ACTIVE_PXX_MIN   950   // Min. value for make PXX control active;
#define ACTIVE_PXX_MAX   1350  // Max. value for make PXX control active;
#define ACTIVE_CPPM_MIN  1650  // Min. value for make CPPM control active;
#define ACTIVE_CPPM_MAX  2050  // Max. value for make CPPM control active;

// Pins definition;
// -----------------------------------------------------------------------

#define LED_PIN        13
#define BEEPER_PIN     7
#define VOLTAGE_PIN    A3
#define R9M_POWER_PIN  3

// R9M power - MOSFET/Relay control inversion;
#define R9M_POWER_OFF HIGH
#define R9M_POWER_ON  LOW

#ifdef OLED
  #define PIN_KEY_NEXT     11
  #define PIN_KEY_SELECT   10
#else
  #define PIN_JUMPER_SETUP 12
#endif

#ifdef RELAY
  #define PIN_CAMERA_SEL   6

  // Camera control to switch 1st and 2nd board camera;
  #define CAMERA_PXX       LOW
  #define CAMERA_CPPM      HIGH
#endif

// Not configurable pins;
// #define RX_PPM_IN_PIN  8  
// #define CPPM_OUT_PIN   9
// #define PXX_OUT_PIN    TX1 

// -----------------------------------------------------------------------

const float LOW_VOLTAGE = 3.47f;
const float MIN_VOLTAGE = 3.33f;
const float correction  = 0.947f;



#endif
