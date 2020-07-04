#ifndef SBUS_h
#define SBUS_h

#include <Arduino.h>
#include "debug.h"
#include "config.h"

#define SBUS_FAILSAFE_INACTIVE 0
#define SBUS_FAILSAFE_ACTIVE   1
#define SBUS_STARTBYTE         0x0f
#define SBUS_ENDBYTE           0x00

#define SBUS_TIMEOUT_MS 11
#define SBUS_BYTES_TIMEOUT_MS 2

// SBUS Channels range
#define SBUS_RANGE_MIN 0
#define SBUS_RANGE_MAX 2047 // 0x7FF

#define MAX_SBUS_CHANNELS 18
#define SBUS_FRAME_LENGTH 25

class SBUS {
	public:
		SBUS(HardwareSerial & serial, unsigned int recoveryTimeMs) : 
			_serial (serial), 
			_recoveryTimeMs(recoveryTimeMs) {};
		void begin();
		bool received(); // post-process and fill shadow channels;
		void receive(); // call from SBUS interrupt;
		bool signalLossActive();
		bool failsafeActive();
		bool isValid() { return _isValid; }

		uint32_t getSignalLossFrames() { return _signalLossFrames; }
		uint32_t getFailsafeFrames() { return _failsafeFrames; }
		uint32_t getFramesCount() { return _framesCount; }
		uint32_t getErrorsCount() { return _errorsCount; }

		void resetStats();

#ifdef DEBUG_SBUS
		uint32_t getInvalidChannelsCount() { return _invalidChannelsCount; }
		uint32_t getMissedFrames() { return _missedFrames; }
		uint32_t getTimeoutsCount() { return _timeoutsCount; }
		uint32_t getOutOfSyncFrames() { return _outOfSyncFrames; }
		uint32_t getBytesCount() { return _bytesCount; }
#endif
		// channels start from 1 to 18
		// returns value 988..2012 (cleanflight friendly)
		uint16_t getChannel(uint8_t channel); // reads shadowed channel (1-based);
		uint16_t getRawChannel(uint8_t channel); // reads raw channel (1-based);
		static bool channelValid(int16_t x) { return (x >= MIN_SERVO_US && x <= MAX_SERVO_US); }
	private:
		HardwareSerial & _serial;
		unsigned int _signalLostTimeMs;

		volatile byte buffer[SBUS_FRAME_LENGTH];
		volatile byte buffer_index = 0;
		volatile int _channels[MAX_SBUS_CHANNELS];
		volatile uint32_t timeoutMs;
		volatile uint32_t prevByteMs;
		volatile bool _gotSBUS;
		volatile bool _signalLossActive;
		volatile bool _failsafeActive;
		volatile bool _hasSBUS;
		volatile bool _oldRx;

		int channels[MAX_SBUS_CHANNELS];

		bool _isValid;
		uint32_t _recoveryTimeMs;

		volatile uint32_t _framesCount; // number of received frames;
		volatile uint32_t _signalLossFrames; // SL frames count;
		volatile uint32_t _failsafeFrames; // FS frames count;
		uint32_t _errorsCount; // number of signal lost, missed or invalid channels conditions longer than recovery time;
		bool _prevHasSBUS; // during the last check SBUS was received;

#ifdef DEBUG_SBUS
		volatile uint32_t _bytesCount; // number of received bytes;
		volatile uint32_t _missedFrames; // number of SBUS timeout in serial stream;
		volatile uint32_t _outOfSyncFrames; // number of SBUS corrupted frames without stop-byte;

		uint32_t _invalidChannelsCount; // number of SBUS frames with channels out of valid range;
		uint32_t _timeoutsCount; // number of SBUS stream interruptions;
#endif
};

#endif
