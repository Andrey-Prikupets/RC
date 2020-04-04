#include "VRx_Rssi.h"

#ifdef SHOW_RSSI

int vrx_rssi_min = VRX_RSSI_MIN;
int vrx_rssi_max = VRX_RSSI_MAX;

void setup_VRx_Rssi() {
  pinMode(VRX_RSSI_PIN, INPUT);           
  digitalWrite(VRX_RSSI_PIN, LOW);

  // Note: moved to main setup() because analog reference is used in another module too;
  // analogReference(INTERNAL); // 1100mV = 1023; built-in reference, equal to 1.1 volts on the ATmega168 or ATmega328P;
}

// get VRx RSSI [%];
int getVRx_Rssi() {
  return map(getVRx_Rssi_raw(), vrx_rssi_min, vrx_rssi_max, 0, 100);
}

#define MIN_ADC_PERIOD 100

int getVRx_Rssi_raw() {
  static long ms = 0;
  static int value = 0;
  if (ms != 0 && millis()-ms < MIN_ADC_PERIOD)
    return value;    
  ms = millis();
  value = analogRead(VRX_RSSI_PIN);
  return value;
}

#endif
