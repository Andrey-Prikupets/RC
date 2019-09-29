#include <Arduino.h>
#include "CPPM.h"
#include "PXX.h"
#include "menu.h"
#include "Seq.h"
#include "MultiTimer.h"
#include "BatteryMonitor.h"
#include "debug.h"

const int LED_PIN = 13;
const int LED_DIVIDER = 10;

const int BEEPER_PIN = 7;
const int VOLTAGE_PIN = A3;
const int R9M_POWER_PIN = 3;

#define R9M_POWER_OFF HIGH
#define R9M_POWER_ON  LOW

const float LOW_VOLTAGE = 3.47f;
const float MIN_VOLTAGE = 3.33f;
const float correction  = 0.947f;
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

#ifdef BEEP_FAILSAFE
NEW_SEQ (SEQ_FAILSAFE,          BEEP_MS(50));
#endif

Seq* seqs[] = {&SEQ_KEY_NEXT, &SEQ_KEY_SELECT, &SEQ_MODE_RANGE_CHECK, &SEQ_MODE_BIND, &SEQ_MODE_CPPM_LOST, &SEQ_MODE_NO_CPPM, &SEQ_MODE_GOT_CPPM, &SEQ_MODE_CPPM_MISSED,
               &SEQ_BATTERY_1S, &SEQ_BATTERY_2S, &SEQ_BATTERY_3S, &SEQ_BATTERY_4S, &SEQ_BATTERY_LOW, &SEQ_BATTERY_MIN
#ifdef BEEP_FAILSAFE
               , &SEQ_FAILSAFE
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
TimerDelay delays[] = {500, 2000, 3000, 200, 1000, 8000, 4000, 4000, 200, 500, 2000};
MultiTimer timer1(delays, sizeof(delays)/sizeof(TimerDelay));

const uint8_t TIMER_MENU_FLASHING    = 0;
const uint8_t TIMER_MODE_SOUND       = 1;
const uint8_t TIMER_NO_CPPM          = 2; 
const uint8_t TIMER_BATTERY_LOOP     = 3; 
const uint8_t TIMER_BATTERY_SCREEN   = 4; 
const uint8_t TIMER_BATTERY_LOW      = 5; 
const uint8_t TIMER_BATTERY_MIN      = 6; 
const uint8_t TIMER_SCREENSAVER      = 7;
const uint8_t TIMER_CHANNELS_SCREEN  = 8;
const uint8_t TIMER_INVALID_FLASHING = 9;
const uint8_t TIMER_CHANNELS_MIN_MAX_FLIP = 10;

BatteryMonitor battery1(VOLTAGE_PIN, LOW_VOLTAGE, MIN_VOLTAGE, DIVIDER, 8, 0.1);

// Servo impulse range where it is valid;
// Note: FrSky X4R in Failsafe with No Pulses setting continues emitting impulses of 800-850us;
#define MIN_SERVO_US 940
#define MAX_SERVO_US 2060

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
    menuSetup();
    showLogo();
    delay(2500);
    CPPM.begin();
#ifndef DEBUG
    PXX.begin();
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
    showScreenSaver();
}

void flashLED() {
    static byte led = HIGH;
    static byte led_count = 0;

  // flash led;
  led_count++;
  if (led_count > LED_DIVIDER) {
    led_count = 0;
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

int16_t channels[16] = { 1500,1500,1000,1500,1200,1400,1600,1800, 1000,1000,1000,1000,1000,1000,1000,1000};
static byte missed_frames = 0;

bool channelsMinMaxSet = false; 
static uint8_t channelsMinMaxLock = 50; // Skip first channel values (they might be unstable); 
int16_t channelsMin[16];
int16_t channelsMax[16];
bool cppmActive = false;

extern bool channelValid(int16_t x);

bool channelValid(int16_t x) {
    return (x >= MIN_SERVO_US && x <= MAX_SERVO_US);
}

void updateChannelsMinMax() {
    if (!channelsMinMaxSet) {
     if (channelsMinMaxLock > 0) {
        channelsMinMaxLock--;
        return;
      }
      channelsMinMaxSet = true;
      memcpy(channelsMin, channels, sizeof(channelsMin));
      memcpy(channelsMax, channels, sizeof(channelsMin));
    } else {
      for(uint8_t i = 0; i < 8; i++) {
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

void doPrepare(bool reset_frames) {
    PXX.prepare(channels);
    if (reset_frames) {
      missed_frames = 0;
      flashLED();
    }
}

#define MAX_ALLOWED_MISSED_FRAMES 10

void loop() {
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
              updateChannelsMinMax();
          }
          if (cppmIsValid) {
              doPrepare(true);
              setCPPM_Obtained();
#ifndef DEBUG
              digitalWrite(R9M_POWER_PIN, R9M_POWER_ON);
#endif
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
#ifndef DEBUG
          digitalWrite(R9M_POWER_PIN, R9M_POWER_OFF);
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
