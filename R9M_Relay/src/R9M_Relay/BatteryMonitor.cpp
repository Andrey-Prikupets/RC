#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor(uint8_t aPin, float aLowCellVoltage, float aMinCellVoltage, float aDivider, uint8_t aFilterSize, float aPrecision) : 
  pin(aPin), divider(aDivider), filterSize(aFilterSize), precision(aPrecision), filterArray(NULL), filterIndex(0), lowVoltage(aLowCellVoltage), minVoltage(aMinCellVoltage),
  adcStarted(false) {
  pinMode(pin, INPUT);
  digitalWrite(pin, LOW);

  // Note: moved to main setup() because analog reference is used in another module too;
  // analogReference(INTERNAL); // 1100mV = 1023; built-in reference, equal to 1.1 volts on the ATmega168 or ATmega328P;
  
  filterArray = new float[filterSize];
}

void BatteryMonitor::begin() {
  if (isCellsDetermined())
    return;
  if (determineNumCells()) {
    lowVoltage *= numCells;
    minVoltage *= numCells;  
#ifdef DEBUG_BATTERY
  Serial.print("begin, low="); Serial.println(lowVoltage);
  Serial.print("begin, min="); Serial.println(minVoltage);
#endif
  }
}

bool BatteryMonitor::determineNumCells() {
  float value;
  filterSum = 0;
  for (byte i=0; i<filterSize; i++) {
    value = getVoltageRaw(false);
    filterArray[i] = value;
    filterSum += value;
  }
  prevVoltage = currVoltage = value;

  for (byte i=0; i<8; i++) {
    value = getVoltage();
    delay(25);
  }
  prevVoltage = currVoltage = value;
#ifdef DEBUG_BATTERY
  Serial.print("determineNumCells="); Serial.println(value);
#endif

  numCells = 0;
  if (value < MIN_CELL_VOLTAGE)
    return false;
  float limit = 0;
  while (value > limit) {
    numCells++;
    limit += MAX_CELL_VOLTAGE;
  }
#ifdef DEBUG_BATTERY
  Serial.print("determineNumCells, cells="); Serial.println((int) numCells);
#endif
  return true;
}

float BatteryMonitor::getVoltage() {
#ifdef DEBUG_BATTERY
  Serial.println();
#endif
  float value = getVoltageRaw(true);
  if (filterSize == 0)
    return value;

  float value1 = (value+filterSum)/(filterSize+1);
  filterIndex++;
  if (filterIndex >= filterSize)
    filterIndex = 0;
  filterSum = filterSum+value-filterArray[filterIndex];
  filterArray[filterIndex] = value;
#ifdef DEBUG_BATTERY
  Serial.print("getVoltage, value1="); Serial.println(value1);
#endif
  return value1;
}

float BatteryMonitor::getVoltageRaw(bool async) {
    int value;
    if (async) {
      if (!adcStarted) {
#ifdef DEBUG_BATTERY
  Serial.print("getVoltageRaw, *async> "); Serial.println(currVoltage);
#endif
        bitSet (ADCSRA, ADSC); // start ADC cycle;
        adcStarted = true;
        return currVoltage;
      } else {
        if (bit_is_clear(ADCSRA, ADSC)) { // ADC clears the bit when cycle is done;
          value = ADC;
          bitSet (ADCSRA, ADSC); // start ADC cycle;
        } else {
#ifdef DEBUG_BATTERY
  Serial.print("getVoltageRaw, !async="); Serial.println(currVoltage);
#endif
          return currVoltage;
        }
      }
    } else {
      value = analogRead(pin);
    }
    float result = value*divider*(1.1f/1023.0f); // 1.1V / 1024;
#ifdef DEBUG_BATTERY
    if (async) {
      Serial.print("getVoltageRaw, async="); Serial.println(result);
    } else {
      Serial.print("getVoltageRaw, sync="); Serial.println(result);
    }
#endif
    return result;
}

#define abs(x) ((x)>0?(x):-(x))

bool BatteryMonitor::isVoltageChanged() {
  bool changed = abs(currVoltage-prevVoltage) >= precision;
#ifdef DEBUG_BATTERY
  if (changed) {
    Serial.print("isVoltageChanged="); Serial.println(currVoltage-prevVoltage);
  }
#endif
  prevVoltage = currVoltage;
  return changed;
}
