#include "watchdog.h"

#ifdef EXTERNAL_WATCHDOG

static int16_t externalWatchdogPin;

#define EXTERNAL_WATCHDOG_RESET_TIME_US 50

void resetExternalWatchdog();

void setupExternalWatchdog(int16_t pin) {
  externalWatchdogPin = pin;
  pinMode(externalWatchdogPin, OUTPUT);
  digitalWrite(externalWatchdogPin, HIGH);

  delayMicroseconds(EXTERNAL_WATCHDOG_RESET_TIME_US);
  resetExternalWatchdog();
}

void resetExternalWatchdog() {
  digitalWrite(externalWatchdogPin, LOW);
  delayMicroseconds(EXTERNAL_WATCHDOG_RESET_TIME_US);
  digitalWrite(externalWatchdogPin, HIGH);
}

#endif

#ifdef WATCHDOG_TIME_MS

void setupInternalWatchdog() {
#ifdef CORE_OFFICIAL
    IWatchdog.begin(WATCHDOG_TIME_MS*1000);
#endif
}

void resetInternalWatchdog() {
#ifdef CORE_OFFICIAL
    IWatchdog.reload();
#endif
}

#endif

#ifdef WATCHDOG_ENABLED

void resetWatchdog() {
#ifdef EXTERNAL_WATCHDOG
  resetExternalWatchdog();
#endif
#ifdef WATCHDOG_TIME_MS
  resetInternalWatchdog();
#endif
}

void delaySafe(uint16_t ms) {
  uint16_t minTimeMs = 0x7FFF;
#ifdef WATCHDOG_TIME_MS
  if (WATCHDOG_TIME_MS < minTimeMs)
    minTimeMs = WATCHDOG_TIME_MS;
#endif
#ifdef EXTERNAL_WATCHDOG
  if (EXTERNAL_WATCHDOG_TIME_MS < minTimeMs)
    minTimeMs = EXTERNAL_WATCHDOG_TIME_MS;
#endif

  while (ms > minTimeMs) {
    resetWatchdog();
    delay(minTimeMs);
    ms -= minTimeMs;
  }

  resetWatchdog();

  if (ms > 0) {
    delay(ms);
    resetWatchdog();
  }  
}

#endif
