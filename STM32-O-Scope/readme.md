# Fork of STM32-O-Scope for ST7735 TFT 128x160 Display

## Changes

* Display changed to ST7735 1'8 TFT 128x160, i.e. https://www.banggood.com/1_8-Inch-TFT-LCD-Display-Module-SPI-Serial-Port-With-4-IO-Driver-p-1164351.html
* Pins mapping changed;
* Screen layout aligned with smaller display;
* Serial command for changing vertical scale added;
* Constant working mode without waiting for trigger signal added;
* Initial yPosition changed to put ground level to the bottom of the screen;
* Showing time removed;

## Pins mapping

TFT_DC	PB0 // RS
TFT_CS	PB1
TFT_RST PA2
TFT_CLK PA5
TFT_SDA PA7

TEST_WAVE_PIN	PB6     
analogInPin 	PA0
