#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "Seq.h"
#include "MultiTimer.h"
#include "BatteryMonitor.h"
#include "debug.h"

// Configurable parameters;
#define MAX_RX_NUM 10 // Up to 255(?), but bigger number requires longer scrolling through RX number menu;
#define BOUNCE_TICK 30 // Delay for keypress settling;

// Configurable pins;
#define PIN_KEY_NEXT 11
#define PIN_KEY_SELECT 10

void menuSetup(void);
void menuLoop(void);
void showLogo(void);
void showScreenSaver(void);
void showChannels(void);

void setCPPM_Start(void);
void setCPPM_Obtained(void);
void setCPPM_Lost(void);

extern BatteryMonitor battery1;
extern Beeper beeper1;
extern MultiTimer timer1;

extern const uint8_t TIMER_MENU_FLASHING;
extern const uint8_t TIMER_MODE_SOUND;
extern const uint8_t TIMER_SCREENSAVER;
extern const uint8_t TIMER_BATTERY_SCREEN;
extern const uint8_t TIMER_NO_CPPM;
extern const uint8_t TIMER_CHANNELS_SCREEN;
extern const uint8_t TIMER_INVALID_FLASHING;

extern Seq SEQ_MODE_RANGE_CHECK;
extern Seq SEQ_MODE_BIND;
extern Seq SEQ_KEY_NEXT;
extern Seq SEQ_KEY_SELECT;
extern Seq SEQ_MODE_NO_CPPM;
extern Seq SEQ_MODE_CPPM_LOST;
extern Seq SEQ_MODE_GOT_CPPM;

extern bool channelValid(int16_t x);

#endif // MENU_H
