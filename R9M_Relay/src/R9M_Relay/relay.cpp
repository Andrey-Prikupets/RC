#include "relay.h"

#ifdef RELAY

int16_t channels_out_pxx [NUM_CHANNELS_PXX] =  { 1500,1500,1000,1500,1200,1400,1600,1800, 1000,1000,1000,1000,1000,1000,1000,1000};
int16_t channels_out_cppm[NUM_CHANNELS_CPPM] = { 1500,1500,1000,1500,1200,1400,1600,1800 };

static int8_t active = RELAY_ACTIVE_NONE;
static int8_t oldActive = RELAY_ACTIVE_NONE;
static bool channelsInitialized = false;

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

void updateChannelsRelay(int16_t channels[]) {
  uint16_t value = channels[RELAY_CHANNEL];
  if (value >= ACTIVE_PXX_MIN && value <= ACTIVE_PXX_MAX) {
    setRelayActive(RELAY_ACTIVE_PXX);
  } else
  if (value >= ACTIVE_CPPM_MIN && value <= ACTIVE_CPPM_MAX) {
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

  if (active != RELAY_ACTIVE_PXX) {
    channels_out_pxx[GPS_MODE_CHANNEL] = GPS_HOLD_VALUE;
  }
  if (active != RELAY_ACTIVE_CPPM) {
    channels_out_cppm[GPS_MODE_CHANNEL] = GPS_HOLD_VALUE;
  }
  channelsInitialized = true;

#ifdef DEBUG_RELAY
  Serial.print("active="); Serial.println(active == RELAY_ACTIVE_PXX ? "PXX" : active == RELAY_ACTIVE_CPPM ? "CPPM" : "---");
  Serial.print("PXX ="); dumpChannels(channels_out_pxx, NUM_CHANNELS_PXX);
  Serial.print("CPPM="); dumpChannels(channels_out_cppm, NUM_CHANNELS_CPPM);
#endif
}

#endif
