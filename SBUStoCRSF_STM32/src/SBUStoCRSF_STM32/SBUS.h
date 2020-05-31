#ifndef SBUS_h
#define SBUS_h

#include <Arduino.h>
#include "debug.h"
#include "config.h"

#define SBUS_FAILSAFE_INACTIVE 0
#define SBUS_FAILSAFE_ACTIVE   1
#define SBUS_STARTBYTE         0x0f
#define SBUS_ENDBYTE           0x00

const unsigned long SBUS_TIMEOUT_MS = 20;

// SBUS Channels range
#define SBUS_RANGE_MIN 0
#define SBUS_RANGE_MAX 2047 // 0x7FF

class SBUS {
	public:
		SBUS(HardwareSerial & serial, unsigned int missedFramesLimit) : 
			_serial (serial), 
			_missedFramesLimit(missedFramesLimit),
			_missedFrames(0),
			_invalidChannelsCount(0),
#ifdef DEBUG_SBUS
			_framesCount(0),
			_bytesCount(0),
#endif    
			_errorsCount(0),
      _timeoutsCount(0),
			_gotSBUS(false) {}
		void begin();
		void process();
		bool signalLossActive() { return _signalLossActive; }
		bool failsafeActive() { return _failsafeActive; }
		bool hasSBUS() { return _hasSBUS; }
		bool isValid() { return _isValid; }
		bool isAcceptable();
		unsigned int getErrorsCount() { return _errorsCount; }
		unsigned int getInvalidChannelsCount() { return _invalidChannelsCount; }
    unsigned int getMissedFrames() { return _missedFrames; }
    unsigned int getTimeoutsCount() { return _timeoutsCount; }
    unsigned int getOutOfSyncFrames() { return _outOfSyncFrames; }
    unsigned int getSignalLossFrames() { return _signalLossFrames; }
    unsigned int getFailsafeFrames() { return _failsafeFrames; }
    unsigned int getMissedFramesOverflows() { return _missedFramesOverflows; }
#ifdef DEBUG_SBUS
		unsigned long getFramesCount();
		unsigned long getBytesCount();
#endif    
		// channels start from 1 to 18
		// returns value 988..2012 (cleanflight friendly)
		uint16_t getChannel(uint8_t channel); // channel is 1-based;
		uint16_t getRawChannel(uint8_t channel); // channel is 1-based;
		static bool channelValid(int16_t x) { return (x >= MIN_SERVO_US && x <= MAX_SERVO_US); }
	private:
		volatile int _channels[18];
		HardwareSerial & _serial;
    unsigned int _missedFramesLimit;
		volatile bool _signalLossActive;
		volatile bool _failsafeActive;
		volatile bool _hasSBUS;
		volatile bool _isValid;
		volatile bool _gotSBUS;
		volatile unsigned long timeoutMs;
		//void check();
#ifdef DEBUG_SBUS
		volatile unsigned long _framesCount;
		volatile unsigned long _bytesCount;
#endif    
		volatile unsigned int _errorsCount;
		volatile unsigned int _missedFrames;
    volatile unsigned int _outOfSyncFrames;
		volatile unsigned int _invalidChannelsCount;
    volatile unsigned int _timeoutsCount;
		volatile unsigned int _signalLossFrames;
    volatile unsigned int _failsafeFrames;
    volatile unsigned int _missedFramesOverflows;
};

#endif
