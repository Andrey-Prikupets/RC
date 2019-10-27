#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "sbus.h" // Used library: 
#include "crossfire.h"
#include "menu.h"
#include "Seq.h"
#include "MultiTimer.h"
#include "BatteryMonitor.h"

#ifndef MCU_STM32F103C8 // ARDUINO_ARCH_STM32F1
  #error This sketch should be flashed to STM32F1 'Blue Pill' device only!
#endif

#ifdef WATCHDOG_TIME_MS
#include "watchdog.h"
#endif

// Pins definitions;

const int8_t LED_PIN = PC13; // NOTE: LED in Blue Pill is inverted;
#define LED_ON  LOW
#define LED_OFF HIGH 
const int8_t LED_DIVIDER = 10; // LED Flasher frequency divider;

const int8_t BEEPER_PIN  = PB12; // 5V buzzer consumes 10mA from 3.3V;
const int8_t VOLTAGE_PIN = PA5; 

// Voltage per cell limits for Battery Low beep and Battery Critically Low beep;
const float LOW_VOLTAGE = 3.47f;
const float MIN_VOLTAGE = 3.33f;

// Voltage meter correction; 
const float correction  = 0.9875; // Fine tune it by measuring real voltage and shown voltage; correction=Vreal/Vmeasured;
const float DIVIDER = (2200.0f+10000.0f)/2200.0f * correction; // Resistor divider: V+ ----| 10k |--- ADC ----| 2.2k | --- GND 

// Beeper sequences;
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

// Multi timers definitions;
// 0  200ms   Battery reading update;
// 1  500ms   Screen update;
// 2  3s      No SBUS alert;
// 3  8s      Battery low alert sound;
// 4  4s      Battery min alert sound;
// 5  500ms   Invalid channels flashing;
TimerDelay delays[] = {200, 500, 3000, 8000, 4000, 500};
MultiTimer mTimer1(delays, sizeof(delays)/sizeof(TimerDelay));

const uint8_t TIMER_BATTERY_LOOP     = 0; 
const uint8_t TIMER_SCREEN_UPDATE    = 1; 
const uint8_t TIMER_NO_SBUS          = 2; 
const uint8_t TIMER_BATTERY_LOW      = 3; 
const uint8_t TIMER_BATTERY_MIN      = 4; 
const uint8_t TIMER_INVALID_FLASHING = 5;

BatteryMonitor battery1(VOLTAGE_PIN, LOW_VOLTAGE, MIN_VOLTAGE, DIVIDER, 8, 0.1);

// Servo impulse range where it is valid;
// Note: FrSky X4R in Failsafe with No Pulses setting continues emitting impulses of 800-850us;
#define MIN_SERVO_US 900
#define MAX_SERVO_US 2100

// Serial ports used;
#define sbusSerial      Serial1 // The default pinout of Serial1 is TX - PA9 ( Arduino D8) and RX - PA10 (Arduino D2).
#define crossfireSerial Serial2 // The default pinout of Serial2 is TX - PA2 ( Arduino D12) and RX - PA3 (Arduino D13).

SBUS sbus (sbusSerial);
CrossfirePulsesData crossfire;

HardwareTimer sbusTimer(1);
HardwareTimer crossfireTimer(2);

bool enableCrossfire = false;

void initChannels();

void setupSBusTimer();
void setupCrossfireTimer();

void setup() {
#ifdef DEBUG
  Serial.begin(DEBUG_BAUD);
#endif
    initChannels();
    menuSetup();
    setupCrossfire(crossfireSerial, &crossfire);
    showLogo();
    delay(LOGO_DELAY_MS);

#ifdef WATCHDOG_TIME_MS
    IWatchdog.begin(WATCHDOG_TIME_MS*1000);
#endif

    sbus.begin();  
    setupSBusTimer();
    setupCrossfireTimer();
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ON);

    battery1.begin();
    mTimer1.start();
    setSBUS_Start();

    switch (battery1.getNumCells()) { 
      case 1: beeper1.play(&SEQ_BATTERY_1S); break;
      case 2: beeper1.play(&SEQ_BATTERY_2S); break;
      case 3: beeper1.play(&SEQ_BATTERY_3S); break;
      case 4: beeper1.play(&SEQ_BATTERY_4S); break;
    }
    showScreenSaver();
}

void sbusProcess()
{
  sbus.process();
}

void crossfireSend()
{
  if (!enableCrossfire)
    return;
    
  sendCrossfireFrame(&crossfire);
}

void setupSBusTimer()
{
  sbusTimer.pause();
  sbusTimer.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
  sbusTimer.setPeriod(SBUS_TIMER_PERIOD_MS);
  sbusTimer.setCompare(TIMER_CH1, 1); // overflow might be small
  sbusTimer.attachInterrupt(TIMER_CH1, sbusProcess);
  sbusTimer.resume();
}

void setupCrossfireTimer() {
  crossfireTimer.pause();
  crossfireTimer.setPeriod(CROSSFIRE_PERIOD*1000);
  crossfireTimer.attachInterrupt(TIMER_UPDATE_INTERRUPT, crossfireSend);  
  crossfireTimer.resume(); // Will be started later when SBUS signal will be valid;
}

void flashLED() {
  static byte led = LED_ON;
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
      if (mTimer1.isTriggered(TIMER_BATTERY_MIN)) {
#ifdef DEBUG_BATTERY
  Serial.println("updateVoltageMonitor, min");
#endif
        beeper1.play(&SEQ_BATTERY_MIN);
      }
  } else
  if (battery1.isLowVoltage()) {
      if (mTimer1.isTriggered(TIMER_BATTERY_LOW)) {
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
    Serial.print("CPPM: ");
    if (!sbus.hasSignal())
      Serial.print("NO SIG ");
#ifdef DEBUG_SBUS
    Serial.print("F: ");
    Serial.print(sbus.getFramesCount(), DEC);
    Serial.print(", B: ");
    Serial.print(sbus.getBytesCount(), DEC);
    Serial.print(", ");
#endif        
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

#ifdef DEBUG_SBUS_RAW_CHANNELS
void debugSBUSRawChannels() {
    Serial.print("SBUS: ");
    if (!sbus.hasSignal())
      Serial.print("NO SIG ");
#ifdef DEBUG_SBUS
    Serial.print("F: ");
    Serial.print(sbus.getFramesCount(), DEC);
    Serial.print(", B: ");
    Serial.print(sbus.getBytesCount(), DEC);
    Serial.print(", ");
#endif        
  for (int i=1; i <= NUM_CHANNELS_SBUS; ++i) {
    Serial.print(i, DEC); 
    Serial.print("=");
    Serial.print(sbus.getRawChannel(i), DEC); 
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
            } else {
              channels[i] = x;              
            }
        }
    }

#ifdef DEBUG_SBUS_RAW_CHANNELS
    debugSBUSRawChannels();
#endif

#ifdef DEBUG_CHANNELS
    debugChannels();
#endif          

    // If sbus channels are invalid, these channels should not be sent to Crossfire, but should be displayed in OLED;
    if (sbusIsValid) {
        doPrepare(true);
        setSBUS_Obtained();
        enableCrossfire = true;        
    } else {
        doPrepare(false);            
    }

    if (sbus.hasSignal() && missed_frames < MAX_ALLOWED_MISSED_FRAMES) {
      if (missed_frames == 1) { // Beep on 1st missed SBUS frame;
        beeper1.play(&SEQ_MODE_SBUS_MISSED);            
      }
      missed_frames++;
    } else {
      setSBUS_Lost();
      enableCrossfire = false;
    }
    
    beeper1.loop();    
    if (mTimer1.isTriggered(TIMER_BATTERY_LOOP)) {
      battery1.loop();
      updateVoltageMonitor();
    }

    menuLoop();

#ifdef WATCHDOG_TIME_MS
    IWatchdog.reload();
#endif

#ifdef DEBUG
#ifndef WATCHDOG_TIME_MS
    delay(1000);
#else
    for (int i=0; i<1000; i++) {
      delay(1);
      IWatchdog.reload();
    }
#endif
#endif    
}
