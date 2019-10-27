#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "config.h"
#include "debug.h"
#include "Seq.h"
#include "MultiTimer.h"
#include "BatteryMonitor.h"

#define VERSION "SBUS-CRSF " VERSION_NUMBER // NOTE: No more than 10 characters for 64x48 OLED display!

void menuSetup(void);
void menuLoop(void);
void showLogo(void);
void showScreenSaver(void);

void setSBUS_Start(void);
void setSBUS_Obtained(void);
void setSBUS_Lost(void);

extern BatteryMonitor battery1;
extern Beeper beeper1;
extern MultiTimer mTimer1;

extern const uint8_t TIMER_SCREEN_UPDATE;
extern const uint8_t TIMER_NO_SBUS;
extern const uint8_t TIMER_CHANNELS_SCREEN;
extern const uint8_t TIMER_INVALID_FLASHING;

extern Seq SEQ_MODE_NO_SBUS;
extern Seq SEQ_MODE_SBUS_LOST;
extern Seq SEQ_MODE_GOT_SBUS;

extern bool channelValid(int16_t x);

extern bool sbusActive;
extern int16_t channels[NUM_CHANNELS_SBUS];

#endif // MENU_H
