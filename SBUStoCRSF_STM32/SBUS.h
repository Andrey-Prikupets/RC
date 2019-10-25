#ifndef SBUS_h
#define SBUS_h

#include <Arduino.h>
#include "debug.h"

#define SBUS_FAILSAFE_INACTIVE 0
#define SBUS_FAILSAFE_ACTIVE   1
#define SBUS_STARTBYTE         0x0f
#define SBUS_ENDBYTE           0x00

const unsigned long SBUS_TIMEOUT_MS = 20;

class SBUS {
	public:
		SBUS(HardwareSerial & serial) : _serial (serial) {}
		void begin();
		void process();
		bool signalLossActive() { return _signalLossActive; }
		bool failsafeActive() { return _failsafeActive; }
		bool hasSignal() { return _hasSignal; }
#ifdef DEBUG_SBUS
    unsigned long getFramesCount();
    unsigned long getBytesCount();
#endif    

		// channels start from 1 to 18
		// returns value 988..2012 (cleanflight friendly)
		uint16_t getChannel(uint8_t channel); // channel is 1-based;
	private:
		volatile int _channels[18];
		HardwareSerial & _serial;
		volatile bool _signalLossActive;
		volatile bool _failsafeActive;
		volatile bool _hasSignal;
    volatile unsigned long timeoutMs;
#ifdef DEBUG_SBUS
    volatile unsigned long _framesCount;
    volatile unsigned long _bytesCount;
#endif    
};

#endif
