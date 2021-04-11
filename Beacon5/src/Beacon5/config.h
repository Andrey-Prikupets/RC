#ifndef CONFIG_H
#define CONFIG_H

#define VERSION 10

// Pins definition;
// -----------------------------------------------------------------------

// LED pins;
#define LED0 7
#define LED1 6
#define LED2 5
#define LED3 4
#define LED4 8

// Button pins;
#define BUTTON1 2
#define BUTTON2 3

// ADC pin;
#define VOLTAGE_PIN A3

// Beeper pin;
#define BEEPER_PIN 9

// -----------------------------------------------------------------------

// Battery voltage parameters;
const float LOW_VOLTAGE = 3.47f; // Critical low voltage per cell;
const float MIN_VOLTAGE = 3.33f; // Low voltage per cell;
const float correction  = 1/1.0108;// Generic is 1.000;

// Voltage divider;
const float DIVIDER = (680.0f+10000.0f)/680.0f * correction; // Resistor divider: V+ ----| 10k |--- ADC ----| 680 | --- GND 

#endif
