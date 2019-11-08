#include "BatteryMonitor.h"
#include "watchdog.h"

BatteryMonitor::BatteryMonitor(uint8_t aPin, float aLowCellVoltage, float aMinCellVoltage, float aDivider, uint8_t aFilterSize, float aPrecision) : 
  pin(aPin), divider(aDivider), filterSize(aFilterSize), precision(aPrecision), filterArray(NULL), filterIndex(0), lowVoltage(aLowCellVoltage), minVoltage(aMinCellVoltage),
  adcStarted(false) {
  pinMode(pin, INPUT_ANALOG);
  // (3.3/4096= 8.056mV) per unit;
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
    value = getVoltageRaw();
    filterArray[i] = value;
    filterSum += value;
  }
  prevVoltage = currVoltage = value;

  for (byte i=0; i<8; i++) {
    value = getVoltage();
    delaySafe(25);
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
  float value = getVoltageRaw();
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

float BatteryMonitor::getVoltageRaw() {
    int value = analogRead(pin);
    float result = value*divider*(3.3f/4096.0f); // 3.3V / 4096;
#ifdef DEBUG_BATTERY
    Serial.print("getVoltageRaw "); Serial.println(result);
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
