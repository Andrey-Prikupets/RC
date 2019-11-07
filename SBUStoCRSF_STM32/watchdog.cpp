#include "watchdog.h"

#ifdef WATCHDOG_TIME_MS
  #include "IWatchdog2.h"
#endif

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
    IWatchdog.begin(WATCHDOG_TIME_MS*1000);
}

void resetInternalWatchdog() {
    IWatchdog.reload();
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
  if (WATCHDOG_TIME_MS/2 < minTimeMs) // Min. Delay is half of WDT period for safety;
    minTimeMs = WATCHDOG_TIME_MS/2;
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
