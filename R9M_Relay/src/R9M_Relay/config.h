#ifndef CONFIG_H
#define CONFIG_H
#include "custom_types.h"

#define VERSION_NUMBER "07"

// Show UI in OLED and support buttons;
// If not defined, serial commands are the only way to configure;
//#define OLED

#define CLI_BAUD  115200

// Enable if needed to show RX RSSI in CLI status command;
#define SHOW_RSSI

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

#ifdef SHOW_RSSI
  #define VRX_RSSI_PIN A6
#endif

// R9M power - MOSFET/Relay control inversion;
// Note: PVDZ172NPbF Solid State Relay connected LED cathode (-) to GND and anode (+) to R9M_POWER_PIN via 220 Ohm resistor;
//       Forward voltage drop = 1.25V, current = 20mA;
//       Output anode (+) goes to battery V+, cathode (-) goes to R9 BEC (+). BEC (-) goes to GND;
#define R9M_POWER_OFF LOW   // HIGH for Arduino relay module; LOW for Solid State Relay (i.e. PVDZ172NPbF);
#define R9M_POWER_ON  HIGH  // LOW for Arduino relay module; HIGH for Solid State Relay (i.e. PVDZ172NPbF);

// CLI and OLED modes are exclusive; OLED is used for ground relay and CLI for airborne relay;
#ifdef OLED
  #define PIN_KEY_NEXT     11
  #define PIN_KEY_SELECT   10
#else
  #define PIN_JUMPER_SETUP 12 // Jumper to GND for enabling CLI (and disabling PXX and R9M);
#endif

#ifdef RELAY
  // Note: Switches cameras via 4052 IC (74HC4052). 
  // Vee(7), INH(6) connected to Vss(8);
  // VRx video out goes to Y0(1), CPPM camera video out goes to Y1(5), video from Yout(3) goes to downlink VTx;
  // Add 0.1uF cap between Vdd(16) and Vss(8);
  #define PIN_CAMERA_SEL   6 // LOW selectes PXX camera, HIGH selects CPPM camera;

  // Camera control to switch CPPM and PXX board camera;
  #define CAMERA_PXX       LOW
  #define CAMERA_CPPM      HIGH
#endif

// Not configurable pins;
// #define RX_PPM_IN_PIN  8  
// #define CPPM_OUT_PIN   9
// #define PXX_OUT_PIN    TX1 

// -----------------------------------------------------------------------

// Battery voltage parameters;
const float LOW_VOLTAGE = 3.47f; // Critical low voltage per cell;
const float MIN_VOLTAGE = 3.33f; // Low voltage per cell;
const float correction  = 0.947f; // Generic is 1.000;

#ifdef SHOW_RSSI
  // Default VRx Rssi parameters;
  #define VRX_RSSI_MIN  136 // 100 is generic value;
  #define VRX_RSSI_MAX  417 // 300 is a generic value;
#endif

#endif
