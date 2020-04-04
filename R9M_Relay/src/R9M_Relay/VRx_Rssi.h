#ifndef VRX_RSSI_H
#define VRX_RSSI_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"

#ifdef SHOW_RSSI

void setup_VRx_Rssi();

// get VRx RSSI [%];
int getVRx_Rssi(); 

// get VRx RSSI ADC value;
int getVRx_Rssi_raw();

extern int vrx_rssi_min;
extern int vrx_rssi_max;

#endif // SHOW_RSSI

#endif // VRX_RSSI_H
