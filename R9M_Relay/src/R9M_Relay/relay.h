#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "PXX.h"
#include "CPPM.h"

#ifdef RELAY

#define RELAY_ACTIVE_NONE 0
#define RELAY_ACTIVE_PXX  1
#define RELAY_ACTIVE_CPPM 2

int8_t getRelayActive();

void updateChannelsRelay(int16_t channels[]);

extern int16_t channels_out_pxx[NUM_CHANNELS_PXX];
extern int16_t channels_out_cppm[NUM_CHANNELS_CPPM];

bool isRelayActiveChanged();

extern Seq SEQ_RELAY_ACTIVE_NONE;
extern Seq SEQ_RELAY_ACTIVE_PXX;
extern Seq SEQ_RELAY_ACTIVE_CPPM;
extern Beeper beeper1;

#ifndef RELAY_CHANNEL
    #error "RELAY_CHANNEL not defined"
#elif RELAY_CHANNEL < CH5
    #error "Unexpected value of RELAY_CHANNEL (less than CH5)"
#endif

#ifndef GPS_MODE_CHANNEL
    #error "GPS_MODE_CHANNEL not defined"
#elif GPS_MODE_CHANNEL < CH5
    #error "Unexpected value of GPS_MODE_CHANNEL (less than CH5)"
#endif

#if RELAY_CHANNEL == GPS_MODE_CHANNEL
    #error "GPS_MODE_CHANNEL must not be same as RELAY_CHANNEL"
#endif

#endif // #ifdef RELAY

#endif // #ifndef RELAY_H
