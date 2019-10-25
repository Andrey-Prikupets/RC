#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "00"

// Comment out when OLED screen is not connected;
#define OLED

// Turn off all sounds;
//#define SOUND_OFF

// Uncomment if need 115200 baud rate instead of 400000;
//#define CROSSFIRE_SLOW_BAUDRATE

// Send only 16 of 18 channels to Crossfire;
// May change to lower numbers if needed;
#define NUM_CHANNELS_SBUS 16

// Channels impulse ranges;
#define CPPM_RANGE_MIN 988
#define CPPM_RANGE_MAX 2012

#define SBUS_TIMER_PERIOD_MS 249 // Check incoming data each 249us = 4 kHz;

#endif
