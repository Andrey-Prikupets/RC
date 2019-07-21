#include "MultiTimer.h"

#ifdef CROSS_DEBUG
#include "Arduino_cross.h"
#else
#include <Arduino.h>
#endif

MultiTimer::MultiTimer(TimerDelay* aDelays, uint8_t aSize) : delays(aDelays), size(aSize), started(false) {
  mss = new unsigned long[size]; 
  suspended = new bool[size];
  memset(suspended, false, size);
}

void MultiTimer::start(void) {
  unsigned long ms = millis();
  for (uint8_t i=0; i<size; i++) {
    if (suspended[i])
      continue;
    mss[i] = ms+delays[i];
#ifdef CROSS_DEBUG
    cout << ms << "; MultiTimer::start index " << (int)i << "[" << delays[i] << "] set to " << mss[i] << "\n";
#endif
  }
  started = true;
}

bool MultiTimer::isTriggered(uint8_t num) {
  if (num >= size) {
#ifdef CROSS_DEBUG
    cout << "MultiTimer::isTriggered index overflow " << (int)num << "\n";
#endif
    return false;
  }
  if (!started || suspended[num])
    return false;

  unsigned long ms = millis();
  if (ms >= mss[num]) {
    mss[num] = ms + delays[num];
#ifdef CROSS_DEBUG
    cout << ms << "; MultiTimer::isTriggered index " << (int)num << "[" << delays[num] << "] set to " << mss[num] << "\n";
#endif
    return true;
  }
  return false;
}

void MultiTimer::suspend(uint8_t num) { 
  if (num >= size) {
#ifdef CROSS_DEBUG
    cout << "MultiTimer::suspend index overflow " << (int)num << "\n";
#endif
    return;
  }
  suspended[num] = true;
}

void MultiTimer::resume(uint8_t num) {
  if (num >= size) {
#ifdef CROSS_DEBUG
    cout << "MultiTimer::resume index overflow " << (int)num << "\n";
#endif
    return;
  }
  mss[num] = millis() + delays[num];
#ifdef CROSS_DEBUG
  cout << ms << "; MultiTimer::resume index " << (int)num << "[" << delays[num] << "] set to " << mss[num] << "\n";
#endif
  suspended[num] = false;
}

void MultiTimer::resetNotSuspended(uint8_t num) {
  if (num >= size) {
#ifdef CROSS_DEBUG
    cout << "MultiTimer::resume index overflow " << (int)num << "\n";
#endif
    return;
  }
  if (suspended[num]) {
#ifdef CROSS_DEBUG
  cout << ms << "; MultiTimer::resetNotSuspended index " << (int)num << "[" << delays[num] << "] suspended " << "\n";
#endif
    return;
  }
  
  mss[num] = millis() + delays[num];
#ifdef CROSS_DEBUG
  cout << ms << "; MultiTimer::resetNotSuspended index " << (int)num << "[" << delays[num] << "] set to " << mss[num] << "\n";
#endif
}
