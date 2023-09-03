#include <math.h>

//#define DEBUG // uncomment to use Serial;

#define FRAME_LEN_MS 10

#define NUM_CHANNELS 7

#define MIN_VALUE 723 // 920ms
// 723/920*1000=786
#define MAX_VALUE 1917 
#define MID_VALUE 1320 // 1519ms

#define FILTER_COEFF 0.75

#define CH_ROLL 1
#define CH_PITCH 2
#define CH_THR 3
#define CH_YAW 4
#define CH_5 5
#define CH_6 6
#define CH_7 7

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

#define CH_POT CH_5

inline void sendToSerial(uint16_t value) {
#ifndef DEBUG
  Serial.write(lowByte(value));
  Serial.write(highByte(value));
#else 
  Serial.print(" ");
  Serial.print(value, HEX);
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
  sendToSerial(HEADER1);
  for (uint8_t i=1; i<=NUM_CHANNELS; i++) {
    sendToSerial(channels[i-1]);
  }
  sendToSerial(FOOTER);
}

void setChannelRawValue(uint8_t channel, float value) {
#ifdef DEBUG
  Serial.print(" CH");
  Serial.print(channel);
  Serial.print("=");
  Serial.print(value);
#endif
  rawChannels[channel-1] = rawChannels[channel-1] * (1-FILTER_COEFF) + value * FILTER_COEFF;
}

float mapToFloat(int x, int in_min, int in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int mapToInt(float x, float in_min, float in_max, int out_min, int out_max) {
  return round((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

float getAnalogRawValue(uint8_t analogPin) {
  int value = analogRead(analogPin);
#ifdef DEBUG
  Serial.print(" A");
  Serial.print(analogPin);
  Serial.print("=");
  Serial.print(value);
#endif
  return mapToFloat(value, 0, 1023, MIN_RAW_VALUE, MAX_RAW_VALUE);
}

float maxAbs(float x1, float x2) {
  return abs(x1) > abs(x2) ? x1 : x2;
}

void readChannels() {
  setChannelRawValue(CH_POT, maxAbs(getAnalogRawValue(PIN_POT1), getAnalogRawValue(PIN_POT2)));
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
#ifdef DEBUG
    Serial.println();
#endif    
  }
}
