#pragma once

#include <stdint.h>
#include "debug.h"

typedef uint16_t TimerDelay;

class MultiTimer
{
public:
  TimerDelay* delays;
  uint8_t size;
  MultiTimer(TimerDelay* aDelays, uint8_t aSize);
  bool isTriggered(uint8_t num);
  bool isStarted() { return started; } 
  void start(void);
  void suspend(uint8_t num);
  void resume(uint8_t num);
  void resetNotSuspended(uint8_t num);
  void stop(void) { started = false; }
private:
  unsigned long* mss;
  bool started;
  bool* suspended;
};
