#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include <sbus.h> // Used library: 
#include "crossfire.h"
#include "menu.h"
#include "Seq.h"
#include "MultiTimer.h"
#include "BatteryMonitor.h"

const int8_t LED_PIN = 13;
const int8_t LED_DIVIDER = 10;

const int8_t BEEPER_PIN = 7;
const int8_t VOLTAGE_PIN = A3;
const int8_t SBUS_PIN = 3; // D3;

const float LOW_VOLTAGE = 3.47f;
const float MIN_VOLTAGE = 3.33f;
const float correction  = 0.947f;
const float DIVIDER = (680.0f+10000.0f)/680.0f * correction; // Resistor divider: V+ ----| 10k |--- ADC ----| 680 | --- GND 

NEW_SEQ (SEQ_MODE_SBUS_LOST,    BEEP_MS(300), PAUSE_MS(70), BEEP_MS(200), PAUSE_MS(50), BEEP_MS(100));
NEW_SEQ (SEQ_MODE_NO_SBUS,      BEEP_MS(30),  PAUSE_MS(20), BEEP_MS(100), PAUSE_MS(50), BEEP_MS(50), PAUSE_MS(20), BEEP_MS(30));
NEW_SEQ (SEQ_MODE_GOT_SBUS,     BEEP_MS(100), PAUSE_MS(50), BEEP_MS(200), PAUSE_MS(70), BEEP_MS(300));
NEW_SEQ (SEQ_MODE_SBUS_MISSED,  BEEP_MS(10));

NEW_SEQ (SEQ_BATTERY_1S,        BEEP_MS(1000));
NEW_SEQ (SEQ_BATTERY_2S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_3S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_4S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_LOW,       BEEP_MS(200), PAUSE_MS(200), BEEP_MS(400), PAUSE_MS(200));
NEW_SEQ (SEQ_BATTERY_MIN,       BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(200));

Seq* seqs[] = {&SEQ_MODE_SBUS_LOST, &SEQ_MODE_NO_SBUS, &SEQ_MODE_GOT_SBUS, &SEQ_MODE_SBUS_MISSED,
               &SEQ_BATTERY_1S, &SEQ_BATTERY_2S, &SEQ_BATTERY_3S, &SEQ_BATTERY_4S, &SEQ_BATTERY_LOW, &SEQ_BATTERY_MIN};
Beeper beeper1(BEEPER_PIN, false, PAUSE_MS(500), 5, seqs, sizeof(seqs)/sizeof(Seq*));

// 0  200ms   Battery reading update;
// 1  500ms   Screen update;
// 2  3s      No SBUS alert;
// 3  8s      Battery low alert sound;
// 4  4s      Battery min alert sound;
// 5  500ms   Invalid channels flashing;
TimerDelay delays[] = {200, 500, 3000, 8000, 4000, 500};
MultiTimer timer1(delays, sizeof(delays)/sizeof(TimerDelay));

const uint8_t TIMER_BATTERY_LOOP     = 0; 
const uint8_t TIMER_SCREEN_UPDATE    = 1; 
const uint8_t TIMER_NO_SBUS          = 2; 
const uint8_t TIMER_BATTERY_LOW      = 3; 
const uint8_t TIMER_BATTERY_MIN      = 4; 
const uint8_t TIMER_INVALID_FLASHING = 5;

BatteryMonitor battery1(VOLTAGE_PIN, LOW_VOLTAGE, MIN_VOLTAGE, DIVIDER, 8, 0.1);

// Servo impulse range where it is valid;
// Note: FrSky X4R in Failsafe with No Pulses setting continues emitting impulses of 800-850us;
#define MIN_SERVO_US 940
#define MAX_SERVO_US 2060

SBUS sbus;
CrossfirePulsesData crossfire;

void initChannels();

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
    initChannels();
    menuSetup();
    setupCrossfire(&crossfire);
    showLogo();
    delay(2500);
    sbus.begin(SBUS_PIN, sbusNonBlocking);  
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    battery1.begin();
    timer1.start();
    setSBUS_Start();

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

// Initially ROLL=PITCH=YAW=1500, THR=1000; Other channels to 1000;
int16_t channels[NUM_CHANNELS_SBUS];

void initChannels() {
  for (uint8_t i=0; i<NUM_CHANNELS_SBUS; i++) {
    int16_t c;
    switch(i) {
      case CH1: 
      case CH2: 
      case CH4: 
        c = 1500;
        break;
      case CH3: 
        c = 1000;
        break;
      default:
        c = 1000;
    }
    channels[i] = c;
  }
}

#ifdef DEBUG_CHANNELS
void debugChannels() {
  for (int i=1; i <= NUM_CHANNELS_SBUS; ++i) {
    Serial.print(i, DEC); 
    Serial.print("=");
    Serial.print(channels[i-1], DEC); 
    Serial.print(" ");
  }
  if (sbus.signalLossActive())
    Serial.print("SL ");
    
  if (sbus.failsafeActive())
    Serial.print("FS");

  Serial.println();
}
#endif

static byte missed_frames = 0;

extern bool channelValid(int16_t x);

bool channelValid(int16_t x) {
    return (x >= MIN_SERVO_US && x <= MAX_SERVO_US);
}

void doPrepare(bool reset_frames) {
    // Mapping channels from CPPM range to CrossFire;
    int16_t standardChannels[NUM_CHANNELS_SBUS];
#ifdef DEBUG_STD_CHANNELS
  Serial.print("CHST> ");  
#endif    
    for (uint8_t i=0; i<NUM_CHANNELS_SBUS; i++) {
      standardChannels[i] = map(channels[i], CPPM_RANGE_MIN, CPPM_RANGE_MAX, CROSSFIRE_RANGE_MIN, CROSSFIRE_RANGE_MAX);
#ifdef DEBUG_STD_CHANNELS
    if (i>0)
      Serial.print(" ");  
    Serial.print((i+1), DEC);  
    Serial.print(":");  
    Serial.print(standardChannels[i], DEC);      
#endif
    }
#ifdef DEBUG_STD_CHANNELS
  Serial.println();  
#endif
    createCrossfireChannelsFrame(&crossfire, standardChannels, NUM_CHANNELS_SBUS);
    if (reset_frames) {
      missed_frames = 0;
      flashLED();
    }
}

#define MAX_ALLOWED_MISSED_FRAMES 10

void loop() {
    bool sbusIsValid = sbus.hasSignal() && !sbus.signalLossActive() && !sbus.failsafeActive();
    if (sbusIsValid) {
        for(uint8_t i = 0; i < NUM_CHANNELS_SBUS; i++) {
            // SBUS range is different from CPPM, but getChannels already returns channels mapped to CPPM range;
            int16_t x = sbus.getChannel(i+1); // 988..2012
            if (!channelValid(x)) {
              sbusIsValid = false; 
            } 
        }
    }
#ifdef DEBUG_CHANNELS
    debugChannels();
#endif          
    // If sbus channels are invalid, these channels should not be sent to Crossfire, but should be displayed in OLED;
    if (sbusIsValid) {
        doPrepare(true);
        setSBUS_Obtained();
    } else {
        doPrepare(false);            
    }

    sendCrossfireFrame(&crossfire, true); // TODO Remove
    
    if (sbus.hasSignal() && missed_frames < MAX_ALLOWED_MISSED_FRAMES) {
      // sendCrossfireFrame(&crossfire, true);
      if (missed_frames == 1) { // Beep on 1st missed SBUS frame;
        beeper1.play(&SEQ_MODE_SBUS_MISSED);            
      }
      missed_frames++;
    } else {
      setSBUS_Lost();
    }
    
    beeper1.loop();    
    if (timer1.isTriggered(TIMER_BATTERY_LOOP)) {
      battery1.loop();
      updateVoltageMonitor();
    }

    menuLoop();
}
