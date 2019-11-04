#ifndef WATCHDOG_h
#define WATCHDOG_h

#include <Arduino.h>
#include "debug.h"
#include "config.h"
#include "checkconfig.h"

// Note: Currently Internal Watchdog is not supported;

#ifdef WATCHDOG_TIME_MS
  #include "IWatchdog2.h"
#endif

#if defined(WATCHDOG_TIME_MS) || defined(EXTERNAL_WATCHDOG)
    #define WATCHDOG_ENABLED
#endif

#ifdef EXTERNAL_WATCHDOG
  #define EXTERNAL_WATCHDOG_TIME_MS 100 // DS1232 TD pin floats; timeout is 250..1000ms, therefore 100ms is a safe value;
  void setupExternalWatchdog(int16_t pin);
#endif

#ifdef WATCHDOG_TIME_MS
  void setupInternalWatchdog();
#endif

#ifdef WATCHDOG_ENABLED
  void delaySafe(uint16_t ms);
  void resetWatchdog();
#else
  #define delaySafe(ms) delay(ms)
  #define resetWatchdog() 
#endif

#endif
