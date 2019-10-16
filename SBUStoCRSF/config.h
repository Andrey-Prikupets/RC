#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "00"

// Comment when OLED screen is not connected;
#define OLED

// Turn off all sounds;
//#define SOUND_OFF

// Uncomment if need 115200 baud rate instead of 400000;
//#define CROSSFIRE_SLOW_BAUDRATE

// Send only 16 of 18 channels to Crossfire;
// May change to lower numbers if needed;
#define NUM_CHANNELS_SBUS 16

#endif
