#ifndef CHECKCONFIG_H

#include <Arduino.h>
#include "config.h"

// MCU_STM32F103C8 - Core STM32F1 from http://dan.drown.org/stm32duino/package_STM32duino_index.json
// ARDUINO_BLUEPILL_F103C8 - Core from https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json
// Note: Last core supports WatchDog but seems does not work;

#ifndef ARDUINO_BLUEPILL_F103C8
  #ifndef MCU_STM32F103C8 
    #error This sketch should be flashed to STM32F1 'Blue Pill' device only. "Official" stm32duino core and "dan.drown.org"/"roger clark" core is supported;
  #else
    #define CORE_DAN_DROWN
  #endif
#else
  #define CORE_OFFICIAL
#endif

#ifdef CORE_OFFICIAL
    #error "Offical" STM32 Arduino core is NOT supported. Please install STM32F1 (STM32Duino.com) so called "dan.drown.org" core.
#endif

#endif