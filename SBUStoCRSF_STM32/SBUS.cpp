#include "SBUS.h"
#include <Arduino.h>

// flag field
#define FLAG_CHANNEL_17  1
#define FLAG_CHANNEL_18  2
#define FLAG_SIGNAL_LOSS 4
#define FLAG_FAILSAFE    8

#define CHANNEL_MIN 173
#define CHANNEL_MAX 1812

#define SBUS_BAUD 100000

void SBUS::begin() {
	for (byte i = 0; i<18; i++) {
		_channels[i]      = 0;
	}
	_signalLossActive = false;
	_failsafeActive = false;
	_hasSignal = false;
  timeoutMs = 0;
	_serial.begin(SBUS_BAUD, SERIAL_8E2);
#ifdef DEBUG_SBUS
 _framesCount = 0;
 _bytesCount = 0;
#endif    
}

void SBUS::process() {
	// while (_serial.available() > 25)
	// {
		// _serial.read();
	// }
	static byte buffer[25];
	static byte buffer_index = 0;

	bool gotData = false;
	while (_serial.available()) {
		byte rx = _serial.read();
#ifdef DEBUG_SBUS
    _bytesCount++;   
#endif    
		if (buffer_index == 0 && rx != SBUS_STARTBYTE) {
			//incorrect start byte, out of sync
			continue;
		}

		buffer[buffer_index++] = rx;

		if (buffer_index == 25) {
			buffer_index = 0;
			if (buffer[24] != SBUS_ENDBYTE) {
				//incorrect end byte, out of sync
				continue;
			}

			_channels[0]  = ((buffer[1]    |buffer[2]<<8)                 & 0x07FF);
			_channels[1]  = ((buffer[2]>>3 |buffer[3]<<5)                 & 0x07FF);
			_channels[2]  = ((buffer[3]>>6 |buffer[4]<<2 |buffer[5]<<10)  & 0x07FF);
			_channels[3]  = ((buffer[5]>>1 |buffer[6]<<7)                 & 0x07FF);
			_channels[4]  = ((buffer[6]>>4 |buffer[7]<<4)                 & 0x07FF);
			_channels[5]  = ((buffer[7]>>7 |buffer[8]<<1 |buffer[9]<<9)   & 0x07FF);
			_channels[6]  = ((buffer[9]>>2 |buffer[10]<<6)                & 0x07FF);
			_channels[7]  = ((buffer[10]>>5|buffer[11]<<3)                & 0x07FF);
			_channels[8]  = ((buffer[12]   |buffer[13]<<8)                & 0x07FF);
			_channels[9]  = ((buffer[13]>>3|buffer[14]<<5)                & 0x07FF);
			_channels[10] = ((buffer[14]>>6|buffer[15]<<2|buffer[16]<<10) & 0x07FF);
			_channels[11] = ((buffer[16]>>1|buffer[17]<<7)                & 0x07FF);
			_channels[12] = ((buffer[17]>>4|buffer[18]<<4)                & 0x07FF);
			_channels[13] = ((buffer[18]>>7|buffer[19]<<1|buffer[20]<<9)  & 0x07FF);
			_channels[14] = ((buffer[20]>>2|buffer[21]<<6)                & 0x07FF);
			_channels[15] = ((buffer[21]>>5|buffer[22]<<3)                & 0x07FF);

			byte b23 = buffer[23];
			_channels[16] = (b23 & FLAG_CHANNEL_17) ? CHANNEL_MAX: CHANNEL_MIN; 
			_channels[17] = (b23 & FLAG_CHANNEL_18) ? CHANNEL_MAX: CHANNEL_MIN; 
			_signalLossActive = b23 & FLAG_SIGNAL_LOSS;
			_failsafeActive = b23 & FLAG_FAILSAFE;
#ifdef DEBUG_SBUS
      _framesCount++;
#endif    
			gotData = true;
		}
	}

	unsigned long ms = millis();
	if (!gotData) {
		_hasSignal = (ms <= timeoutMs);		
	} else {
		timeoutMs = millis()+SBUS_TIMEOUT_MS;
    _hasSignal = true;
	}
}

// channels start from 1 to 18
// returns value 988..2012 (cleanflight friendly)
uint16_t SBUS::getChannel(uint8_t channel) { // channel is 1-based;
  uint8_t ch = channel-1;
  noInterrupts(); 
  uint16_t  result = _channels[ch]; 
  interrupts();
  return 5 * result / 8 + 880;
}

unsigned long SBUS::getFramesCount() { 
  noInterrupts(); 
  unsigned long result = _framesCount; 
  interrupts();
  return result;
}

unsigned long SBUS::getBytesCount() { 
  noInterrupts(); 
  unsigned long result = _bytesCount; 
  interrupts();
  return result;
}

