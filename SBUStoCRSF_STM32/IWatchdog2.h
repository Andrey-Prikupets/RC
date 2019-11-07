#ifndef IWATCHDOG2_h
#define IWATCHDOG2_h

// IWatchdog - from https://github.com/stm32duino/Arduino_Core_STM32

#include <Arduino.h>
#include "config.h"
#include "checkconfig.h"
#include "debug.h"
#include "STM32F103_core_part.h"

// Minimal timeout in microseconds
#define IWDG_TIMEOUT_MIN    ((4*1000000)/LSI_VALUE)
// Maximal timeout in microseconds
#define IWDG_TIMEOUT_MAX    (((256*1000000)/LSI_VALUE)*IWDG_RLR_RL)

#define IS_IWDG_TIMEOUT(X)  (((X) >= IWDG_TIMEOUT_MIN) &&\
                             ((X) <= IWDG_TIMEOUT_MAX))

class IWatchdogClass {

  public:
    void begin(uint32_t timeout, uint32_t window = IWDG_TIMEOUT_MAX);
    void set(uint32_t timeout, uint32_t window = IWDG_TIMEOUT_MAX);
    void get(uint32_t *timeout, uint32_t *window = NULL);
    void reload(void);
    bool isEnabled(void)
    {
      return _enabled;
    };
    bool isReset(bool clear = false);
    void clearReset(void);

  private:
    static bool _enabled;
};

extern IWatchdogClass IWatchdog;

#endif
