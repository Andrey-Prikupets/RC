#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "config.h"
#include "debug.h"

const float MAX_CELL_VOLTAGE = 4.2f;
const float MIN_CELL_VOLTAGE = 3.0f;

class BatteryMonitor
{
public:
  BatteryMonitor(uint8_t aPin, float aLowCellVoltage, float aMinCellVoltage, float aDivider, uint8_t aFilterSize, float aPrecision);
  uint8_t getNumCells() { return numCells; }
  float getCurrVoltage() { return currVoltage; }
  void begin();
  void loop(void) { currVoltage = getVoltage(); }
  bool isVoltageChanged();
  bool isLowVoltage(void) { return isCellsDetermined() && currVoltage <= lowVoltage; } 
  bool isMinVoltage(void) { return isCellsDetermined() && currVoltage <= minVoltage; } 
  bool isCellsDetermined() { return numCells > 0; } 
private:
  uint8_t numCells;
  uint8_t pin;
  float lowVoltage;
  float minVoltage;
  float* filterArray; // Circular buffer;
  uint8_t filterSize; 
  float filterSum;
  uint8_t filterIndex; // Buffer last filled position;
  float divider;
  float precision;
  float prevVoltage;
  float currVoltage;
  bool adcStarted;
  bool  determineNumCells();
  float getVoltage();
  float getVoltageRaw();
};
