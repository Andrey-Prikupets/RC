#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "PXX.h"
#include "CPPM.h"

#ifdef RELAY

// Configurable pins;
#define PIN_CAMERA_PXX 6

#define CAMERA_PXX  LOW
#define CAMERA_CPPM HIGH

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

extern bool    relayEnabled;
extern uint8_t relayChannel;     // Channel to switch between PXX and PPM control; Allowed only channels CH5..CH8;
extern uint8_t gpsModeChannel;   // Set it to channel to enable GPS HOLD mode; Allowed only channels CH5..CH8;
extern uint16_t gpsHoldValue;    // Set it to enable GPS HOLD in GPS_MODE_CHANNEL on both relay and mission drone;

// RELAY_CHANNEL signal boundaries to enable PXX or CPPM control;
extern uint16_t activePXX_Min;   // Min. value for make PXX control active;
extern uint16_t activePXX_Max;   // Max. value for make PXX control active;
extern uint16_t activeCPPM_Min;  // Min. value for make CPPM control active;
extern uint16_t activeCPPM_Max;  // Max. value for make CPPM control active;

#endif // #ifdef RELAY

#endif // #ifndef RELAY_H
