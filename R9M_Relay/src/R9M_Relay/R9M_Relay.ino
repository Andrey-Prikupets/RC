#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "CPPM.h"
#include "PXX.h"
#include "menu.h"
#include "Seq.h"
#include "MultiTimer.h"
#include "BatteryMonitor.h"
#include "relay.h"
#include "VRx_Rssi.h"

const int LED_DIVIDER = 10; // Arduino board LED flashing period divider;

// Voltage divider;
const float DIVIDER = (680.0f+10000.0f)/680.0f * correction; // Resistor divider: V+ ----| 10k |--- ADC ----| 680 | --- GND 

NEW_SEQ (SEQ_KEY_NEXT,          BEEP_MS(30));
NEW_SEQ (SEQ_KEY_SELECT,        BEEP_MS(30), PAUSE_MS(20), BEEP_MS(30));
NEW_SEQ (SEQ_MODE_RANGE_CHECK,  BEEP_MS(200));
NEW_SEQ (SEQ_MODE_BIND,         BEEP_MS(100), PAUSE_MS(30), BEEP_MS(100));
NEW_SEQ (SEQ_MODE_CPPM_LOST,    BEEP_MS(300), PAUSE_MS(70), BEEP_MS(200), PAUSE_MS(50), BEEP_MS(100));
NEW_SEQ (SEQ_MODE_NO_CPPM,      BEEP_MS(30),  PAUSE_MS(20), BEEP_MS(100), PAUSE_MS(50), BEEP_MS(50), PAUSE_MS(20), BEEP_MS(30));
NEW_SEQ (SEQ_MODE_GOT_CPPM,     BEEP_MS(100), PAUSE_MS(50), BEEP_MS(200), PAUSE_MS(70), BEEP_MS(300));
NEW_SEQ (SEQ_MODE_CPPM_MISSED,  BEEP_MS(10));

NEW_SEQ (SEQ_BATTERY_1S,        BEEP_MS(1000));
NEW_SEQ (SEQ_BATTERY_2S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_3S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_4S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_LOW,       BEEP_MS(200), PAUSE_MS(200), BEEP_MS(400), PAUSE_MS(200));
NEW_SEQ (SEQ_BATTERY_MIN,       BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(200));

#ifdef RELAY
NEW_SEQ (SEQ_RELAY_ACTIVE_NONE, BEEP_MS(100), PAUSE_MS(50),  BEEP_MS(200), PAUSE_MS(50), BEEP_MS(200));
NEW_SEQ (SEQ_RELAY_ACTIVE_PXX,  BEEP_MS(300), PAUSE_MS(100), BEEP_MS(50));
NEW_SEQ (SEQ_RELAY_ACTIVE_CPPM, BEEP_MS(50),  PAUSE_MS(100), BEEP_MS(300));
#endif

#ifdef BEEP_FAILSAFE
NEW_SEQ (SEQ_FAILSAFE,          BEEP_MS(50));
#endif

Seq* seqs[] = {&SEQ_KEY_NEXT, &SEQ_KEY_SELECT, &SEQ_MODE_RANGE_CHECK, &SEQ_MODE_BIND, &SEQ_MODE_CPPM_LOST, &SEQ_MODE_NO_CPPM, &SEQ_MODE_GOT_CPPM, &SEQ_MODE_CPPM_MISSED,
               &SEQ_BATTERY_1S, &SEQ_BATTERY_2S, &SEQ_BATTERY_3S, &SEQ_BATTERY_4S, &SEQ_BATTERY_LOW, &SEQ_BATTERY_MIN
#ifdef BEEP_FAILSAFE
               , &SEQ_FAILSAFE
#endif               
#ifdef RELAY
               , &SEQ_RELAY_ACTIVE_NONE
               , &SEQ_RELAY_ACTIVE_PXX
               , &SEQ_RELAY_ACTIVE_CPPM
#endif
               };
Beeper beeper1(BEEPER_PIN, false, PAUSE_MS(500), 5, seqs, sizeof(seqs)/sizeof(Seq*));

// 0  500ms   Menu flashing;
// 1  2s      Range check and Bind mode sound;
// 2  3s      Range check and Bind mode sound;
// 3  200ms   Battery reading update;
// 4  1s      Battery screen update;
// 5  8s      Battery low alert sound;
// 6  4s      Battery min alert sound;
// 7  4s      Screensaver out of menu delay;
// 8  200ms   Channels update;
// 9  500ms   Invalid channels flashing;
//10  2000ms  Channels Min/Max page flip;
//11  250ms   LED normal flashing;
//12  50ms    LED flashing no-signal; 
//13  1000ms  CLI periodic functions output; 
TimerDelay delays[] = {500, 2000, 3000, 200, 1000, 8000, 4000, 4000, 200, 500, 2000, 250, 50, 1000};
MultiTimer timer1(delays, sizeof(delays)/sizeof(TimerDelay));

const uint8_t TIMER_MENU_FLASHING         = 0;
const uint8_t TIMER_MODE_SOUND            = 1;
const uint8_t TIMER_NO_CPPM               = 2; 
const uint8_t TIMER_BATTERY_LOOP          = 3; 
const uint8_t TIMER_BATTERY_SCREEN        = 4; 
const uint8_t TIMER_BATTERY_LOW           = 5; 
const uint8_t TIMER_BATTERY_MIN           = 6; 
const uint8_t TIMER_SCREENSAVER           = 7;
const uint8_t TIMER_CHANNELS_SCREEN       = 8;
const uint8_t TIMER_INVALID_FLASHING      = 9;
const uint8_t TIMER_CHANNELS_MIN_MAX_FLIP = 10;
const uint8_t TIMER_LED_VALID_FLASHING    = 11;
const uint8_t TIMER_LED_INVALID_FLASHING  = 12;
const uint8_t TIMER_CLI_PERIODIC          = 13;

BatteryMonitor battery1(VOLTAGE_PIN, LOW_VOLTAGE, MIN_VOLTAGE, DIVIDER, 8, 0.1);

// Servo impulse range where it is valid;
// Note: FrSky X4R in Failsafe with No Pulses setting continues emitting impulses of 800-850us;
#define MIN_SERVO_US 940
#define MAX_SERVO_US 2060

void setup() {
#ifdef DEBUG
  Serial.begin(DEBUG_BAUD);
  Serial.println(F("DEBUG"));
#endif

  // All analog inputs (Battery voltage and VRx RSSI) use internal analog reference 1.1v;
  analogReference(INTERNAL); // 1100mV = 1023; built-in reference, equal to 1.1 volts on the ATmega168 or ATmega328P;

#ifdef RELAY
    relayInit();
#endif
#ifdef SHOW_RSSI
    setup_VRx_Rssi();
#endif    
    menuSetup();
#ifdef OLED    
    showLogo();
#endif    
    delay(500);
    CPPM.begin();
#ifdef OLED
    PXX.begin(); // When CLI is defined, handleCli handles PXX.begin;
#endif
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    pinMode(R9M_POWER_PIN, OUTPUT);
    digitalWrite(R9M_POWER_PIN, R9M_POWER_OFF);  

    battery1.begin();
    timer1.start();
    setCPPM_Start();

    switch (battery1.getNumCells()) { 
      case 1: beeper1.play(&SEQ_BATTERY_1S); break;
      case 2: beeper1.play(&SEQ_BATTERY_2S); break;
      case 3: beeper1.play(&SEQ_BATTERY_3S); break;
      case 4: beeper1.play(&SEQ_BATTERY_4S); break;
    }
#ifdef OLED    
    showScreenSaver();
#endif    
}

void flashLED(bool valid) {
  static byte led = HIGH;

  if (timer1.isTriggered(valid ? TIMER_LED_VALID_FLASHING : TIMER_LED_INVALID_FLASHING)) {
    led = !led;
    digitalWrite(LED_PIN, led);
  }
}

void updateVoltageMonitor() {
  if (battery1.isMinVoltage()) {
      if (timer1.isTriggered(TIMER_BATTERY_MIN)) {
#ifdef DEBUG_BATTERY
  Serial.println("updateVoltageMonitor, min");
#endif
        beeper1.play(&SEQ_BATTERY_MIN);
      }
  } else
  if (battery1.isLowVoltage()) {
      if (timer1.isTriggered(TIMER_BATTERY_LOW)) {
#ifdef DEBUG_BATTERY
  Serial.println("updateVoltageMonitor, low");
#endif
        beeper1.play(&SEQ_BATTERY_LOW);
      }
  }  
}

int16_t channels[NUM_CHANNELS_PXX] = { 1500,1500,1000,1500,1200,1400,1600,1800, 1000,1000,1000,1000,1000,1000,1000,1000};

static byte missed_frames = 0;

#ifdef RC_MIN_MAX
bool channelsMinMaxSet = false; 
static uint8_t channelsMinMaxLock = 50; // Skip first channel values (they might be unstable); 
int16_t channelsMin[NUM_CHANNELS_CPPM];
int16_t channelsMax[NUM_CHANNELS_CPPM];
#endif

bool cppmActive = false;

extern bool channelValid(int16_t x);

bool channelValid(int16_t x) {
    return (x >= MIN_SERVO_US && x <= MAX_SERVO_US);
}

#ifdef RC_MIN_MAX
void updateChannelsMinMax() {
    if (!channelsMinMaxSet) {
     if (channelsMinMaxLock > 0) {
        channelsMinMaxLock--;
        return;
      }
      channelsMinMaxSet = true;
      memcpy(channelsMin, channels, NUM_CHANNELS_CPPM*sizeof(channels[0]));
      memcpy(channelsMax, channels, NUM_CHANNELS_CPPM*sizeof(channels[0]));
    } else {
      for(uint8_t i = 0; i < NUM_CHANNELS_CPPM; i++) {
        int16_t x = channels[i];
        if (channelsMin[i] > x) {
          channelsMin[i] = x;  
        }
        if (channelsMax[i] < x) {
          channelsMax[i] = x;  
        }
      }
    }
}
#endif

void doPrepare(bool reset_frames) {
    PXX.prepare(
#ifdef RELAY
      channels_out_pxx
#else
      channels
#endif
    );
#if defined(EMIT_CPPM) || defined(RELAY)    
    // Send channels to CPPM out;
    for (uint8_t i=0; i<NUM_CHANNELS_CPPM; i++) {
      CPPM.write_us(i, 
#ifdef RELAY
      channels_out_cppm[i]
#else
      channels[i]
#endif
      ); 
    }
#endif
    if (reset_frames) {
      missed_frames = 0;
    }
    flashLED(reset_frames);
}

void enableR9M(boolean enable) {
  enum R9M_state {
    UNDEFINED=0, R9M_ENABLED=1, R9M_DISABLED=2  
  };
  static uint8_t state = UNDEFINED;
  uint8_t newState = enable && !isCliActive() ? R9M_ENABLED : R9M_DISABLED;
  if (state != newState) {
#ifndef DEBUG
    digitalWrite(R9M_POWER_PIN, newState == R9M_ENABLED ? R9M_POWER_ON : R9M_POWER_OFF);
#else
    state = newState;
    Serial.print(F("R9M "));
    Serial.println(enable ? F("enabled") : F("disabled"));
#endif
  }
}

#ifndef OLED
void handleCLI() {
  static bool initialized = false;
  bool enabled = !digitalRead(PIN_JUMPER_SETUP); // LOW = enabled;
  if (enabled != !PXX.isStarted() || !initialized) {
    initialized = true;
    if (enabled) {
      PXX.end();
#ifndef DEBUG
      Serial.begin(CLI_BAUD);
#endif
      enableR9M(false);
      Serial.println(F("CLI is ready. Type 'help' for help..."));
      setCliActive(true);
    } else {
      Serial.println(F("Bye"));
      setCliActive(false);
      PXX.begin();
    }
  }
}
#endif

#define MAX_ALLOWED_MISSED_FRAMES 10

void loop() {
#ifndef OLED
    handleCLI();
#endif
  
    static bool prepare = true;

    if(prepare) {
        if (PXX.getModeBind()) { // AP; send PXX if BIND pressed;
           doPrepare(true);
#ifndef DEBUG
           digitalWrite(R9M_POWER_PIN, R9M_POWER_ON); // Power R9M during BIND regardless of CPPM state;
#endif
        } else {
          CPPM.cycle();
          bool cppmIsValid = cppmActive = CPPM.synchronized();
          if (cppmIsValid) {
              for(uint8_t i = 0; i < 8; i++) {
                  int16_t x = channels[i] = CPPM.read_us(i);
                  if (!channelValid(x)) {
                    cppmIsValid = false; 
                  } 
              }
#ifdef RC_MIN_MAX
              updateChannelsMinMax();
#endif        
          }
          if (cppmIsValid) {
              doPrepare(true);
              setCPPM_Obtained();
#ifdef RELAY
              updateChannelsRelay(channels);
              CPPM.enableOutput(true);
#endif              
              enableR9M(true);
          } else {
              doPrepare(false);            
          }
        }

        menuLoop();        
    } else {
        if (missed_frames < MAX_ALLOWED_MISSED_FRAMES) {
#ifndef DEBUG
          PXX.send();
#endif
          if (missed_frames == 1) { // Beep on 1st missed CPPM frame;
            beeper1.play(&SEQ_MODE_CPPM_MISSED);            
          }
          missed_frames++;
        } else {
          setCPPM_Lost();
          enableR9M(false);
#ifdef RELAY
          CPPM.enableOutput(false);
#endif
        }
    }
    
    beeper1.loop();    
    if (timer1.isTriggered(TIMER_BATTERY_LOOP)) {
      battery1.loop();
      updateVoltageMonitor();
    }

    prepare = !prepare;

    delay(2); // 2+2 ms additional pause between PXX packets;
}
