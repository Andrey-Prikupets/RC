
#include "relay.h"

#ifdef RELAY

bool relayEnabled = RELAY_ENABLED;
uint8_t relayChannel = RELAY_CHANNEL;      // Channel to switch between PXX and PPM control; Allowed only channels CH5..CH8;
uint8_t gpsModeChannel = GPS_MODE_CHANNEL; // Set it to channel to enable GPS HOLD mode; Allowed only channels CH5..CH8;
uint16_t gpsHoldValue = GPS_HOLD_VALUE;    // Set it to enable GPS HOLD in GPS_MODE_CHANNEL on both relay and mission drone;

// RELAY_CHANNEL signal boundaries to enable PXX or CPPM control;
uint16_t activePXX_Min = ACTIVE_PXX_MIN;   // Min. value for make PXX control active;
uint16_t activePXX_Max = ACTIVE_PXX_MAX;   // Max. value for make PXX control active;
uint16_t activeCPPM_Min = ACTIVE_CPPM_MIN; // Min. value for make CPPM control active;
uint16_t activeCPPM_Max = ACTIVE_CPPM_MAX; // Max. value for make CPPM control active;

// CH5 = 1000 for ARM_CPPM = 0;
// CH6 = 1000 for ARM_PXX = 0;
int16_t channels_out_pxx [NUM_CHANNELS_PXX]  = { 1500,1500,1000,1500,1000,1000,1000,1000, 1000,1000,1000,1000,1000,1000,1000,1000};
int16_t channels_out_cppm[NUM_CHANNELS_CPPM] = { 1500,1500,1000,1500,1000,1000,1000,1000 };

static int8_t active = RELAY_ACTIVE_NONE;
static int8_t oldActive = RELAY_ACTIVE_NONE;
static bool channelsInitialized = false;

void relayInit() {
  pinMode(PIN_CAMERA_SEL, OUTPUT);
  digitalWrite(PIN_CAMERA_SEL, CAMERA_CPPM);
}

int8_t getRelayActive() {
  return active;
}

bool isRelayActiveChanged() {
  if (oldActive == active)
    return false;

  oldActive = active;
  return true;
}

void setRelayActive(int8_t aActive) {
  if (aActive == active) {
    return;
  }

  active = aActive;
  beeper1.play(active == RELAY_ACTIVE_PXX ?  &SEQ_RELAY_ACTIVE_PXX : 
               active == RELAY_ACTIVE_CPPM ? &SEQ_RELAY_ACTIVE_CPPM : 
                                             &SEQ_RELAY_ACTIVE_NONE);            
}

#ifdef DEBUG_RELAY
void dumpChannels(int16_t channels[], int8_t size) {
  for (int8_t i=0; i<size; i++) {
    if (i > 0) Serial.print(","); 
    Serial.print(channels[i], DEC);     
  }
  Serial.println();
}
#endif

void centerControlsAndHold(int16_t channels[]) {
  // Enter GPS Hold;
  channels[gpsModeChannel] = gpsHoldValue;

  // Reset Roll, Pitch, Yaw;
  // Throttle is in last position;
  channels[CH1] = 1500;
  channels[CH2] = 1500;
  channels[CH4] = 1500;
}

void updateChannelsRelay(int16_t channels[]) {
  uint16_t value = channels[relayChannel];
  if (relayEnabled && (value >= activePXX_Min && value <= activePXX_Max)) {
    setRelayActive(RELAY_ACTIVE_PXX);
  } else
  if (relayEnabled && (value >= activeCPPM_Min && value <= activeCPPM_Max)) {
    setRelayActive(RELAY_ACTIVE_CPPM);
  } else {
    setRelayActive(RELAY_ACTIVE_NONE);
  }

  if (active == RELAY_ACTIVE_PXX || !channelsInitialized) {
    memcpy(channels_out_pxx, channels, NUM_CHANNELS_PXX*sizeof(channels[0]));
  } 
  if (active == RELAY_ACTIVE_CPPM || !channelsInitialized) {
    memcpy(channels_out_cppm, channels, NUM_CHANNELS_CPPM*sizeof(channels[0]));  
  }

  if (relayEnabled) {
    if (active != RELAY_ACTIVE_PXX) {
      centerControlsAndHold(channels_out_pxx);
    }
    if (active != RELAY_ACTIVE_CPPM) {
      centerControlsAndHold(channels_out_cppm);
    }

    if (active == RELAY_ACTIVE_PXX) {
      digitalWrite(PIN_CAMERA_SEL, CAMERA_PXX);
    }
    if (active == RELAY_ACTIVE_CPPM) {
      digitalWrite(PIN_CAMERA_SEL, CAMERA_CPPM);
    } else {
      // If both CPPM and PXX copters are inactive, keep last selected camera;
    }
  }

  channelsInitialized = true;

#ifdef DEBUG_RELAY
  Serial.print("active="); Serial.println(active == RELAY_ACTIVE_PXX ? "PXX" : active == RELAY_ACTIVE_CPPM ? "CPPM" : "---");
  Serial.print("PXX ="); dumpChannels(channels_out_pxx, NUM_CHANNELS_PXX);
  Serial.print("CPPM="); dumpChannels(channels_out_cppm, NUM_CHANNELS_CPPM);
#endif
}

#endif
