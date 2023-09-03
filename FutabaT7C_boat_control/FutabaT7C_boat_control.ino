#include <math.h>

//#define DEBUG // if in DEBUG, radio commands are not sent to Serial, and it is used for debug output;
//#define DEBUG_RADIO // debug radio communication;
//#define DEBUG_ADC
//#define DEBUG_CHANNELS

//#define REVERSE_POT1
#define REVERSE_POT2

#define FRAME_LEN_MS 10

#define NUM_CHANNELS 7 // DO NOT CHANGE!

#define MIN_VALUE 723 // 920ms
// 723/920*1000=786
#define MAX_VALUE 1917 
#define MID_VALUE 1320 // 1519ms

#define FILTER_COEFF_SLOW 0.1
#define FILTER_COEFF_FAST 0.75

#define CH_1 1
#define CH_2 2
#define CH_3 3
#define CH_4 4

#define PIN_POT1 A7
#define PIN_POT2 A6

static long frameStartMs;

const uint16_t HEADER1 = 0xFFFE;
const uint16_t HEADER2 = 0xFFFC;
const uint16_t FOOTER  = 0x03C3;

#define MIN_RAW_VALUE (-100.0)
#define MAX_RAW_VALUE 100.0
#define MID_RAW_VALUE 0.0

uint16_t channels[NUM_CHANNELS];
float rawChannels[NUM_CHANNELS]; // -100..100;

#define CH_THR    CH_1
#define CH_YAW    CH_2
#define CH_SERVO1 CH_3
#define CH_SERVO2 CH_4

// comment out if reverse not needed;
#define REVERSE_YAW
#define REVERSE_SERVO1
//#define REVERSE_SERVO2

// ideally ADC reading in center is 1024/2=512;
#define POT1_CENTER 508
#define POT2_CENTER 504

// Turn off debugging features if global DEBUG is off;
#ifndef DEBUG
#undef DEBUG_RADIO
#undef DEBUG_ADC
#undef DEBUG_CHANNELS
#endif

inline void sendToSerial(uint16_t value) {
#ifndef DEBUG
  Serial.write(lowByte(value));
  Serial.write(highByte(value));
#else 
#ifdef DEBUG_RADIO
  Serial.print(" ");
  Serial.print(value, HEX);
#endif
#endif
}

void setup() {
  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);

#ifndef DEBUG
  Serial.begin(50000, SERIAL_8E1);
#else 
  Serial.begin(115200);
#endif
  for (uint8_t i=1; i<=NUM_CHANNELS; i++) {
    rawChannels[i-1] = i == CH_THR ? MIN_RAW_VALUE : MID_RAW_VALUE;
  }
  frameStartMs = millis()+FRAME_LEN_MS;
}

void outputChannels() {
#ifdef DEBUG_RADIO
  Serial.print("Radio: ");
#endif

  sendToSerial(HEADER1);
  for (uint8_t i=1; i<=NUM_CHANNELS; i++) {
    sendToSerial(channels[i-1]);
  }
  sendToSerial(FOOTER);

#ifdef DEBUG_RADIO
  Serial.println();
#endif
}

void setChannelRawValue(uint8_t channel, float value, bool slow) {
  float filterCoeff = slow ? FILTER_COEFF_SLOW : FILTER_COEFF_FAST;
  float result = rawChannels[channel-1] * (1-filterCoeff) + value * filterCoeff;
#ifdef DEBUG_CHANNELS
  Serial.print(" CH");
  Serial.print(channel);
  Serial.print("=");
  Serial.print(value);
  Serial.print(" -> ");
  Serial.print(result);
#endif
  rawChannels[channel-1] = result;
}

float mapToFloat(int16_t x, int16_t in_min, int16_t in_max, float out_min, float out_max) {
  x = constrain(x, in_min, in_max);
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void swapFloat(float &a, float &b) {
  float c = a;
  a = b;
  b = c;
}

float mapToFloatWithReverse(float x, float in_min, float in_max, float out_min, float out_max, bool reverse) {
  if (reverse) {
    swapFloat(out_min, out_max);
  }
  return mapToFloat(x, in_min, in_max, out_min, out_max);
}

int16_t mapToInt(float x, float in_min, float in_max, int16_t out_min, int16_t out_max) {
  x = constrainFloat(x, in_min, in_max);
  return round((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

float constrainFloat(float x, float in_min, float in_max) {
  return (x < in_min) ? in_min : ((x > in_max) ? in_max : x);
}

float getAnalogRawValue(uint8_t analogPin, int16_t center) {
  int16_t value = analogRead(analogPin);
  float result = mapToFloat(value-center, -1024/2, 1024/2, MIN_RAW_VALUE, MAX_RAW_VALUE);

#ifdef DEBUG_ADC
  Serial.print(" A");
  Serial.print(analogPin);
  Serial.print("=");
  Serial.print(value);
  Serial.print(" -> ");
  Serial.print(result);
#endif
  return result;
}

void readChannels() {
  // Map POT1 0..100 to Throttle -100..100;
  // Map POT1 -25..-50 to Servo1 -100 -> +100 smooth;
  // Map POT1 -75..-100 to Servo2 -100 -> +100 smooth;  
  // Map POT2 -100..100 to Yaw -100..100;
  
  float pot1 = getAnalogRawValue(PIN_POT1, POT1_CENTER);
  float pot2 = getAnalogRawValue(PIN_POT2, POT2_CENTER);

#ifdef REVERSE_POT1
  pot1 = -pot1;
#endif

#ifdef REVERSE_POT2
  pot2 = -pot2;
#endif

#ifdef REVERSE_YAW // might me XOR with REVERSE_POT2 instead but anyway;
  pot2 = -pot2;
#endif

#ifdef REVERSE_SERVO1
  bool reverse1 = true;
#else 
  bool reverse1 = false;
#endif

#ifdef REVERSE_SERVO2
  bool reverse2 = true;
#else 
  bool reverse2 = false;
#endif

  setChannelRawValue(CH_THR, mapToFloat(pot1, 0, 100, -100, 100), false);
  setChannelRawValue(CH_SERVO1, mapToFloatWithReverse(-pot1, 25,  50, -100, 100, reverse1), true);
  setChannelRawValue(CH_SERVO2, mapToFloatWithReverse(-pot1, 75, 100, -100, 100, reverse2), true);
  setChannelRawValue(CH_YAW, pot2, false);

#if defined(DEBUG_CHANNELS) || defined(DEBUG_ADC)
  Serial.println();
#endif
}

void prepareChannels() {
  for (uint8_t i=1; i<=NUM_CHANNELS; i++) {
    channels[i-1] = mapToInt(rawChannels[i-1], MIN_RAW_VALUE, MAX_RAW_VALUE, MIN_VALUE, MAX_VALUE);
  }  
}

void loop() {
  readChannels();
  if (frameStartMs < millis()) {
    frameStartMs = millis()+FRAME_LEN_MS;
    prepareChannels();
    outputChannels();
  }
}
