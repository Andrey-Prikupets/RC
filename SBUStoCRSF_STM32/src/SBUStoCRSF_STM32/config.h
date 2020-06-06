#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "02"

// Comment out when OLED screen is not connected;
#define OLED

// Turn off all sounds;
//#define SOUND_OFF

// Voltage per cell limits for Battery Low beep and Battery Critically Low beep;
const float LOW_VOLTAGE = 3.47f;
const float MIN_VOLTAGE = 3.33f;

// Voltage meter correction; 
//const float correction  = 1.045; // Fine tune it by measuring real voltage and shown voltage; correction=Vreal/Vmeasured;
const float correction  = 1.045; // for Dmitry A.;
const float DIVIDER = (2200.0f+10000.0f)/2200.0f * correction; // Resistor divider: V+ ----| 10k |--- ADC ----| 2.2k | --- GND 

// Uncomment if need 115200 baud rate instead of 400000;
//#define CROSSFIRE_SLOW_BAUDRATE

// Send only 16 of 18 channels to Crossfire;
// May change to lower numbers if needed;
#define NUM_CHANNELS_SBUS 16

// Timeout value for consequent missed or signal lost SBUS frames or frames with invalid channel values to disable CrossFire and renders them to its FailSafe;
// Note: FailSafe condition in SBUS reflects in CrossFire FailSafe without delay;
#define SBUS_RECOVERY_TIME_MS 200

// Servo impulse range where it is valid;
// Note: FrSky X4R in Failsafe with No Pulses setting continues emitting impulses of 800-850us;
#define MIN_SERVO_US 900
#define MAX_SERVO_US 2100

// Standard CPPM Channels impulse ranges that SBUS can handle;
#define CPPM_RANGE_MIN 857
#define CPPM_RANGE_MAX 2143

// Conversion from SBUS to internal CPPM parameters;

//#define SBUS_TO_CPPM_CENTER_SHIFT 0 // set to 0 if no shift is needed;
//#define SBUS_TO_CPPM_CENTER_SHIFT +3 // XLite + X4R;
#define SBUS_TO_CPPM_CENTER_SHIFT +20 // Hours + XSR;

//#define SBUS_TO_CPPM_EXTEND_COEFF 1.0 // > 1 - extend CPPM range when converting from SBUS; < 1 - shrink it;
#define SBUS_TO_CPPM_EXTEND_COEFF 0.9945 // Horus + XSR;

// Conversion from internal CPPM to CRSF parameters;

#define CPPM_TO_CRSF_CENTER_SHIFT 0 // set to 0 if no shift is needed;

//#define CPPM_TO_CRSF_EXTEND_COEFF 1.0 //  > 1 - extend CRSF range when converting from CPPM; < 1 - shrink it;
#define CPPM_TO_CRSF_EXTEND_COEFF 1.2593 //  CRSF to Betaflight;

// SBUS reading settings;

#define SBUS_TIMER_PERIOD_US 249 // Check incoming data each 249us = 4 kHz;

// Watchdog settings;

#define WATCHDOG_TIME_MS 500

// Enable if external watchdog DS1232 is used; comment out if not; 
// It has 3 configurable timeouts (62.5-250ms, 250-1000ms, 500-2000ms);
// It is recommended to set it up to 250-1000ms minimal timeout because it is refreshed in the main loop
// and if OSD is enabled the loop time may be up to 100ms;
// Note: Bootloader must have no start-up delay, otherwise ext. WDT will reset before start of the real program;
//#define EXTERNAL_WATCHDOG

#define LOGO_DELAY_MS 1000

#endif
