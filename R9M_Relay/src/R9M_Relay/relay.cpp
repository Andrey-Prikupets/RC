
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

bool     holdThrottleEnabled = HOLD_THROTTLE_ENABLED; // Enable setting mid throttle (normally, 1500) for armed inactive copter and min throttle for disarmed inactive copter;
uint16_t midThrottle = MID_THROTTLE;       // Mid throttle value;
uint16_t minThrottle = MIN_THROTTLE;       // Min throttle value;

// ARM channel signal boundaries for PXX or CPPM control; only armed copter will receive mid throttle when inactive; not armed will receive min throtlle;
uint8_t  armCPPMChannel = ARM_CPPM_CHANNEL; // Set it to channel to arm CPPM controlled copter; Allowed only channels CH5..CH8;
uint8_t  armPXXChannel  = ARM_PXX_CHANNEL;  // Set it to channel to arm PXX controlled copter; Allowed only channels CH5..CH8;
uint16_t armCPPM_Min    = ARM_CPPM_MIN;     // Min. value for make CPPM arm;
uint16_t armCPPM_Max    = ARM_CPPM_MAX;     // Max. value for make CPPM arm;
uint16_t armPXX_Min     = ARM_PXX_MIN;      // Min. value for make PXX arm;
uint16_t armPXX_Max     = ARM_PXX_MAX;      // Max. value for make PXX arm;

// CH5 = 1000 for ARM_CPPM = 0;
// CH6 = 1000 for ARM_PXX = 0;
int16_t channels_out_pxx [NUM_CHANNELS_PXX]  = { 1500,1500,1000,1500,1000,1000,1000,1000, 1000,1000,1000,1000,1000,1000,1000,1000};
int16_t channels_out_cppm[NUM_CHANNELS_CPPM] = { 1500,1500,1000,1500,1000,1000,1000,1000 };

static int8_t active = RELAY_ACTIVE_NONE;
static int8_t oldActive = RELAY_ACTIVE_NONE;
static bool channelsInitialized = false;
static bool pxx_armed = false;
static bool cppm_armed = false;

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

bool inRange(uint16_t x, uint16_t min, uint16_t max) {
  return x >= min && x <= max;
}

void centerControlsAndHold(int16_t channels[], bool thrOverride, uint16_t thrOverrideValue) {
  // Enter GPS Hold;
  channels[gpsModeChannel] = gpsHoldValue;

  // Reset Roll, Pitch, Yaw;
  // Throttle is in last position;
  channels[CH1] = 1500;
  channels[CH2] = 1500;
  if (thrOverride) {
    channels[CH3] = thrOverrideValue;
  }
  channels[CH4] = 1500;
}

template <typename T>
class Debouncer {
public:
  Debouncer(int8_t maxCount) : maxCount(maxCount), counter(0), initialized(false) {};
  T debounce(T value) {
    if (!initialized) {
      initialized = true;
      stableValue = value;
      prevValue = value;
    } else
    if (prevValue == value) {
      if (counter < maxCount) {
        counter++;
      } else {
        stableValue = value;
      }
    } else { // value == stableValue || value != prevValue;
      prevValue = value;
      counter = 0;
    }
    return stableValue;
  }
private:
  int8_t maxCount;
  bool initialized;
  int8_t counter;
  T stableValue;
  T prevValue;
};

static Debouncer<bool> pxx_debouncer (ARM_DEBOUNCE_FRAMES);
static Debouncer<bool> cppm_debouncer(ARM_DEBOUNCE_FRAMES);

void updateChannelsRelay(int16_t channels[]) {
  pxx_armed = channelsInitialized && pxx_debouncer.debounce(inRange(channels[armPXXChannel], armPXX_Min, armPXX_Max));
  cppm_armed = channelsInitialized && cppm_debouncer.debounce(inRange(channels[armCPPMChannel], armCPPM_Min, armCPPM_Max));

  uint16_t value = channels[relayChannel];
  if (relayEnabled && inRange(value, activePXX_Min, activePXX_Max)) {
    setRelayActive(RELAY_ACTIVE_PXX);
  } else
  if (relayEnabled && inRange(value, activeCPPM_Min, activeCPPM_Max)) {
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
      centerControlsAndHold(channels_out_pxx, holdThrottleEnabled, pxx_armed ? midThrottle : minThrottle);
    }
    if (active != RELAY_ACTIVE_CPPM) {
      centerControlsAndHold(channels_out_cppm, holdThrottleEnabled, cppm_armed ? midThrottle : minThrottle);
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

bool getCPPMArmed() {
  return cppm_armed;
}

bool getPXXArmed() {
  return pxx_armed;
}

#endif
