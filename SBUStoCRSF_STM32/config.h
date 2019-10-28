#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "01"

// Comment out when OLED screen is not connected;
#define OLED

// Turn off all sounds;
//#define SOUND_OFF

// Uncomment if need 115200 baud rate instead of 400000;
//#define CROSSFIRE_SLOW_BAUDRATE

// Send only 16 of 18 channels to Crossfire;
// May change to lower numbers if needed;
#define NUM_CHANNELS_SBUS 16

// Standard CPPM Channels impulse ranges that SBUS can handle;
#define CPPM_RANGE_MIN 857
#define CPPM_RANGE_MAX 2143
#define SBUS_TO_CPPM_CENTER_SHIFT +3 // set to 0 if no shift is needed;

#define SBUS_TIMER_PERIOD_MS 249 // Check incoming data each 249us = 4 kHz;

// Disable if Watchdog not needed or IWatchdog library not available;
// IWatchdog - from https://github.com/stm32duino/Arduino_Core_STM32
//#define WATCHDOG_TIME_MS 500

// Enable if external watchdog DS1232 is used; comment out if not; 
// It has 3 configurable timeouts (62.5-250ms, 250-1000ms, 500-2000ms);
// It is recommended to set it up to 250-1000ms minimal timeout because it is refreshed in the main loop
// and if OSD is enabled the loop time may be up to 100ms;
#define EXTERNAL_WATCHDOG

#define LOGO_DELAY_MS 1000

#endif
