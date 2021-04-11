#include <EEPROM.h>
#include <Button2.h>

#include "config.h"
#include "BatteryMonitor.h"
#include "MultiTimer.h"
#include "Seq.h"

/*
TODO
  Add SBUS support to enable flashing by defined channel;
  Leave long left click to switch to beacon mode;
  Add long right click to switch to flashlight mode;
  Add flashlight mode that lights up top LED0 with configured PWM;
  Add flashlight mode that lights up side LED1 with configured PWM;
  Add flashlight mode that lights up all LEDs with configured PWM;
  Make speed change in flashlight mode to change PWN;
  Read and store PWM;
  Change version;
*/

Button2 button1(BUTTON1);
Button2 button2(BUTTON2);

uint8_t leds[] = {LED0, LED1, LED2, LED3, LED4};

#define NUM_LEDS (sizeof(leds)/sizeof(leds[0]))

#define LN (0)
#define L0 (1 << 0)
#define L1 (1 << 1)
#define L2 (1 << 2)
#define L3 (1 << 3)
#define L4 (1 << 4)

#define D(n) (n/10)

#define STOP {0,0}

struct __attribute__ ((packed)) Step {
  uint8_t wait10ms : 8;
  uint8_t leds_mask : NUM_LEDS;
};

// Initial flash;
// Lights each LED starting from top for 100ms;
// Loop time: 0.4s;
static Step INTRO[] = { {D(500), L0}, {D(500), L1}, {D(500), L2}, {D(500), L3}, {D(500), L4}, STOP };

// Flashes all slowly with interval 0.5s;
// Loop time: 1s;
static Step FLASH_ALL[] = { {D(500), L0+L1+L2+L3+L4}, {D(500), LN}, STOP };

// Flashes all faster with interval 0.25s;
// Loop time: 0.5s;
//static Step FLASH_ALL_FAST[] = { {D(250), L0+L1+L2+L3+L4}, {D(250), LN}, STOP };

// Lights top and another LED, then short pause, then top and next LED, then pause etc.;
// Loop time: 1s;
static Step ROUND_FLASH_WITH_TOP[] = { {D(200), L0+L1}, {D(50), LN}, {D(200), L0+L2}, {D(50), LN}, {D(200), L0+L3}, {D(50), LN}, {D(200), L0+L4}, {D(50), LN}, STOP };

// Lights top and another LED, then turns off top LED and lights another LED, than light top LED and another LED etc.;
// Loop time: 1s;
static Step ROUND_FLASH_TWO[] = { {D(250), L0+L1}, {D(250), L2}, {D(250), L0+L3}, {D(250), L4}, STOP };

// Lights top LED, than turns it off and lights another LED etc.;
// Loop time: 1s;
static Step ROUND_FLASH_FIVE[] = { {D(200), L0}, {D(200), L1}, {D(200), L2}, {D(200), L3}, {D(200), L4}, STOP };

// Lights LED one by one without turning it off, top LED lights last, then pause;
// Loop time: 1.2s;
static Step ROUND_FLASH_ACCUM[] = { {D(200), L1}, {D(200), L1+L2}, {D(200), L1+L2+L3}, {D(200), L1+L2+L3+L4}, {D(200), L1+L2+L3+L4+L0}, {D(200), LN}, STOP };

// Lights cross LED, then add another cross, then adds top, then pause;
// Loop time: 1s;
static Step ROUND_FLASH_ACCUM_CROSS[] = { {D(250), L1+L3}, {D(250), L1+L2+L3+L4}, {D(250), L1+L2+L3+L4+L0}, {D(250), LN}, STOP };

// Lights cross LED, then turns it off and another cross, top flashes faster;
// Loop time: 1s;
static Step ROUND_FLASH_CROSS[] = { {D(250), L1+L3},  {D(250), L1+L3+L0},  {D(250), L2+L4},  {D(250), L2+L4+L0}, STOP };

// Lights cross LED, then turns it off and another cross, top flashes faster (twice as faster);
// Loop time: 1s;
//static Step ROUND_FLASH_CROSS_FAST[] = { {D(120), L1+L3},  {D(130), L1+L3+L0},  {D(120), L2+L4},  {D(130), L2+L4+L0}, STOP };

static Step* FLASHES[] = {
  FLASH_ALL,
  // FLASH_ALL_FAST, 
  ROUND_FLASH_WITH_TOP, 
  ROUND_FLASH_TWO, 
  ROUND_FLASH_FIVE, 
  ROUND_FLASH_ACCUM, 
  ROUND_FLASH_ACCUM_CROSS, 
  ROUND_FLASH_CROSS
  // ROUND_FLASH_CROSS_FAST
};

#define MAX_SEQ (sizeof(FLASHES)/sizeof(FLASHES[0])-1)

#define MIN_SPEED 0
#define NORMAL_SPEED 2

static uint8_t SPEEDS[] = {2, 5, 10, 20, 40};

#define MAX_SPEED (sizeof(SPEEDS)/sizeof(SPEEDS[0])-1)

// Beeps -----------------------------------------------------

NEW_SEQ (SEQ_BATTERY_1S,        BEEP_MS(1000));
NEW_SEQ (SEQ_BATTERY_2S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_3S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_4S,        BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200));
NEW_SEQ (SEQ_BATTERY_LOW,       BEEP_MS(200), PAUSE_MS(200), BEEP_MS(400), PAUSE_MS(200));
NEW_SEQ (SEQ_BATTERY_MIN,       BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(100), BEEP_MS(200), PAUSE_MS(200));

Seq* seqs[] = { &SEQ_BATTERY_1S, &SEQ_BATTERY_2S, &SEQ_BATTERY_3S, &SEQ_BATTERY_4S, &SEQ_BATTERY_LOW, &SEQ_BATTERY_MIN };
Beeper beeper1(BEEPER_PIN, false, PAUSE_MS(500), 5, seqs, sizeof(seqs)/sizeof(Seq*));

// 0  200ms   Battery reading update;
// 1  1s      Battery screen update;
// 2  8s      Battery low alert sound;
// 3  4s      Battery min alert sound;
TimerDelay delays[] = {200, 1000, 8000, 4000};
MultiTimer timer1(delays, sizeof(delays)/sizeof(TimerDelay));

const uint8_t TIMER_BATTERY_LOOP          = 0; 
const uint8_t TIMER_BATTERY_SCREEN        = 1; 
const uint8_t TIMER_BATTERY_LOW           = 2; 
const uint8_t TIMER_BATTERY_MIN           = 3; 

BatteryMonitor battery1(VOLTAGE_PIN, LOW_VOLTAGE, MIN_VOLTAGE, DIVIDER, 8, 0.1);

//-- Variables ------------------------------------------------

static uint8_t flashNum = 0;
static uint8_t step = 0;
static boolean flashing = true;
static uint8_t speed = NORMAL_SPEED;
static unsigned long finishDelayMs;
static boolean inDelay = false;

//-------------------------------------------------------------

void setup() {
  for (uint8_t i=0; i<NUM_LEDS; i++) {
    pinMode(leds[i], OUTPUT);
  }

  button1.setLongClickHandler(longClick);
  button1.setClickHandler(click);
  button1.setDoubleClickHandler(doubleClick);
  button1.setTripleClickHandler(tripleClick);

  button2.setLongClickHandler(longClick);
  button2.setClickHandler(click);
  button2.setDoubleClickHandler(doubleClick);
  button2.setTripleClickHandler(tripleClick);

#ifdef DEBUG
  Serial.begin(DEBUG_BAUD);
  Serial.println(F("DEBUG"));
#endif

  delay(500);

  flashOnce(INTRO);

  battery1.begin();
  timer1.start();

  readConfig();

  switch (battery1.getNumCells()) { 
    case 1: beeper1.play(&SEQ_BATTERY_1S); break;
    case 2: beeper1.play(&SEQ_BATTERY_2S); break;
    case 3: beeper1.play(&SEQ_BATTERY_3S); break;
    case 4: beeper1.play(&SEQ_BATTERY_4S); break;
  }
#ifdef DEBUG_FLASH
  Serial.println();
#endif  
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

#define VERSION_ADDR   0
#define FLASH_ADDR     1
#define SPEED_ADDR     2

void readConfig() {
  uint8_t version = EEPROM.read(VERSION_ADDR);

  if (version == VERSION) {
    flashNum = EEPROM.read(FLASH_ADDR);
    speed = EEPROM.read(SPEED_ADDR);
  }

  if (flashNum > MAX_SEQ) {
    flashNum = 0;
  }

  if (speed > MAX_SPEED) {
    speed = NORMAL_SPEED;
  }
#ifdef DEBUG_FLASH
  Serial.print("read, flashNum="); Serial.println(flashNum);
  Serial.print("read, speed="); Serial.println(speed);
#endif  
}

void writeConfig() {
  EEPROM.update(VERSION_ADDR, (uint8_t) VERSION);
  
#ifdef DEBUG_FLASH
  Serial.print("write, flashNum="); Serial.println(flashNum);
  Serial.print("write, speed="); Serial.println(speed);
#endif  
  EEPROM.update(FLASH_ADDR, flashNum);
  EEPROM.update(SPEED_ADDR, speed);
}

void flashOnce(Step flash[]) {
  uint8_t i = 0;
  while (flash[i].wait10ms > 0) {
#ifdef DEBUG_FLASH
  Serial.print("flashOnce, i="); Serial.println(i);
#endif  
    setLEDs(flash[i].leds_mask);
    delay10ms(flash[i].wait10ms, true);
    i++;
  }
  setLEDs(LN);
}

void flashLoop() {
  if (!flashing) {
    return;
  }

  if (isInDelay()) {
    return;
  }

#ifdef DEBUG_FLASH
  Serial.print("flashLoop, flashNum="); Serial.print(flashNum);
  Serial.print(", step="); Serial.println(step);
#endif  
  Step* flash = FLASHES[flashNum];
  if (flash[step].wait10ms == 0) { // STOP codon reached, looping to step=0;
    step = 0;
#ifdef DEBUG_FLASH
  Serial.println("  step set 0");
#endif  
  }
  setLEDs(flash[step].leds_mask);
  delay10ms(flash[step].wait10ms, false);
  step++;
}

void setLEDs(uint8_t mask) {
#ifdef DEBUG_FLASH
  Serial.print("  setLEDs, mask="); Serial.println(mask, BIN);
#endif  
  for (uint8_t i=0; i<NUM_LEDS; i++) {
#ifdef DEBUG_FLASH
  Serial.print("  LED "); Serial.print(leds[i]); Serial.print("="); Serial.println(bitRead(mask,i));
#endif  
    digitalWrite(leds[i], bitRead(mask,i));
  }  
}

boolean isInDelay() {
  if (!inDelay) {
    return false;
  }
  if (millis() > finishDelayMs) {
    inDelay = false;
    return false;
  }
  return true;
}

void delay10ms(uint8_t wait10ms, boolean sync) {
  unsigned long ms = ((int)wait10ms) * SPEEDS[speed];
#ifdef DEBUG_FLASH
  Serial.print("  delay10ms, wait10ms="); Serial.print(wait10ms);
  Serial.print(", speed="); Serial.print(speed);
  Serial.print(", sync="); Serial.print(sync);
  Serial.print(", delay="); Serial.println(ms);
#endif  
  if (sync) {
    delay(ms);
  } else {
    finishDelayMs = millis()+ms;
#ifdef DEBUG_FLASH
  Serial.print("  finishDelayMs="); Serial.println(finishDelayMs);
#endif  
    inDelay = true;
  }
}

void clearDelay() {
  inDelay = false;  
}

void click(Button2& b) {
  if (b == button1) {
#ifdef DEBUG_BUTTONS
  Serial.print("click b1, flashNum="); Serial.print(flashNum);
#endif  
    if (flashNum >= MAX_SEQ) {
      flashNum = 0;
    } else {
      flashNum++;
    }
  } else {
#ifdef DEBUG_BUTTONS
  Serial.print("click b2, flashNum="); Serial.print(flashNum);
#endif  
    if (flashNum == 0) {
      flashNum = MAX_SEQ;
    } else {
      flashNum--;
    }
  }
  writeConfig();
#ifdef DEBUG_BUTTONS
  Serial.print(" to "); Serial.println(flashNum);
#endif  
  step = 0;
  clearDelay();
}

void longClick(Button2& b) {
#ifdef DEBUG_BUTTONS
  Serial.print("longClick, flashing="); Serial.print(flashing);
#endif  
  flashing = !flashing;
  setLEDs(LN);
#ifdef DEBUG_BUTTONS
  Serial.print(" to "); Serial.println(flashing);
#endif  
  step = 0;
  clearDelay();
}

void doubleClick(Button2& b) {
  if (b == button1) {
#ifdef DEBUG_BUTTONS
  Serial.print("doubleClick, b1, speed="); Serial.print(speed);
#endif  
    if (speed < MAX_SPEED) {
      speed++;
    }
  } else {
#ifdef DEBUG_BUTTONS
  Serial.print("doubleClick, b2, speed="); Serial.print(speed);
#endif  
    if (speed > 0) {
      speed--;
    }
  }
#ifdef DEBUG_BUTTONS
  Serial.print(" to "); Serial.println(speed);
#endif  
  writeConfig();
}

void tripleClick(Button2& b) {
#ifdef DEBUG_BUTTONS
  Serial.print("tripleClick, speed="); Serial.print(speed);
  Serial.print(" to "); Serial.println(NORMAL_SPEED);
#endif  
  speed = NORMAL_SPEED;
  writeConfig();
}

void loop() {
  button1.loop();
  button2.loop();
  
  flashLoop();

  beeper1.loop();    

  if (timer1.isTriggered(TIMER_BATTERY_LOOP)) {
      battery1.loop();
      updateVoltageMonitor();
  }
}
