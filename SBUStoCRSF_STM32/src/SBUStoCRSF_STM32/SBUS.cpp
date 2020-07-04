#include "SBUS.h"
#include <math.h>
#include <limits.h>

// flag field
#define FLAG_CHANNEL_17  1
#define FLAG_CHANNEL_18  2
#define FLAG_SIGNAL_LOSS 4
#define FLAG_FAILSAFE    8

#define CHANNEL_MIN 173
#define CHANNEL_MAX 1812

#define SBUS_BAUD 100000

const float SBUS_CENTER = (SBUS_RANGE_MIN+SBUS_RANGE_MAX)/2.0;
const float SBUS_HALF_RANGE = (SBUS_RANGE_MAX-SBUS_RANGE_MIN)/2.0;
const float CPPM_CENTER = (CPPM_RANGE_MAX+CPPM_RANGE_MIN)/2.0;
const float CPPM_HALF_RANGE = (CPPM_RANGE_MAX-CPPM_RANGE_MIN)/2.0;
const float CPPM_HALF_RANGE_CORR = CPPM_HALF_RANGE*SBUS_TO_CPPM_EXTEND_COEFF;
const float CPPM_CENTER_CORR = CPPM_CENTER+SBUS_TO_CPPM_CENTER_SHIFT;
const float SBUS_TO_CPPM_CORR = CPPM_HALF_RANGE_CORR/SBUS_HALF_RANGE;

#define CPPM_TO_SBUS(x) ((((float)(x)-CPPM_CENTER_CORR) / SBUS_TO_CPPM_CORR)+SBUS_CENTER)

const int SBUS_VALID_MIN = (int) round(CPPM_TO_SBUS(MIN_SERVO_US));
const int SBUS_VALID_MAX = (int) round(CPPM_TO_SBUS(MAX_SERVO_US));

void SBUS::begin() {
	for (byte i = 0; i<18; i++) {
		_channels[i] = 0;
	}
	_signalLossActive = false;
	_failsafeActive = false;
	_hasSBUS = false;
	timeoutMs = 0;
	_serial.begin(SBUS_BAUD, SERIAL_8E2);
	resetStats();
	oldRx = 0;
}

void SBUS::resetStats() {
	framesCount = _signalLossFrames = _failsafeFrames = _errorsCount = 
#ifdef DEBUG_SBUS
		_bytesCount = _missedFrames = _outOfSyncFrames = _invalidChannelsCount = _timeoutsCount =
#endif    
		0;
}

void SBUS::receive() {
	bool frameReceived = false;
 
	if (millis()-prevByteMs > SBUS_BYTES_TIMEOUT_MS && buffer_index != 0) { // If too much time passed between bytes, it's a gap; reset read buffer;
		buffer_index == 0;
		_failsafeFrames++;
	}

	while (_serial.available() && !frameReceived) {
		byte rx = _serial.read();
#ifdef DEBUG_SBUS
		_bytesCount++;   
#endif
		prevByteMs = millis();

		bool waitForHeader = buffer_index == 0 && (rx != SBUS_STARTBYTE || _oldRx != SBUS_ENDBYTE);
		_oldRx = rx;
		if (waitForHeader) {
			//incorrect start byte, out of sync
			continue;
		}

		// if Header just found, reset the SBUS timeout; whole packet should be received in 10 ms;
		if (buffer_index == 0) {
		timeoutMs = millis()+SBUS_TIMEOUT_MS;
		}
		
		buffer[buffer_index++] = rx;

		if (buffer_index == SBUS_FRAME_LENGTH) {
			buffer_index = 0;
			if (buffer[SBUS_FRAME_LENGTH-1] != SBUS_ENDBYTE) { // Sometimes 129 = 0x81
				//incorrect end byte, out of sync
#ifdef DEBUG_SBUS
				_outOfSyncFrames++;
#endif
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
			if (_failsafeActive) {
				_failsafeFrames++;
			} else
			if (_signalLossActive) {
				_signalLossFrames++;
			}
			_framesCount++;
			frameReceived = true;
		}
	}

	if (!frameReceived) {
		_hasSBUS = millis() <= timeoutMs;	// false if have not received SBUS frame within the timeout;	
		if (!_hasSBUS) {
			// clean up the buffer and UART FIFO;
			buffer_index = 0;
 //     while (_serial.available()) {
 //       _serial.read();
 //      }
#ifdef DEBUG_SBUS
			_missedFrames++;
#endif
		}
	} else {
		timeoutMs = millis()+SBUS_TIMEOUT_MS;
		buffer_index = 0;
		_hasSBUS = true;
		_gotSBUS = true;
	}
}

bool SBUS::received() {
	bool prevValid = _isValid;
	_isValid = false;

	noInterrupts(); 
	bool lostSBUS = !_hasSBUS && _prevHasSBUS;
	_prevHasSBUS = _hasSBUS;
	if (lostSBUS) {
#ifdef DEBUG_SBUS
		_timeoutsCount++; // Serial connection timeout - was RX disconnected?  
#endif
	}

	if (!_gotSBUS || _failsafeActive) {
		interrupts();
		return _isValid; // return if not ever received valid SBUS frame or in case of FS;
	}

	bool hasChannels = _hasSBUS && !_signalLossActive;
	bool channelsValid = true;
	if (hasChannels) {
		for(uint8_t i = 0; i < 15; i++) { // Channels 16 and 17 are not checked;
			int x = _channels[i];      
			if (x < SBUS_VALID_MIN || x > SBUS_VALID_MAX) {
				channelsValid = false; 
#ifdef DEBUG_SBUS
				_invalidChannelsCount++; // Invalid channels errors;
#endif
				break;
			}
		}
	}

	if (hasChannels && channelsValid) {
		_isValid = true;
		// Copy channels to their shadow storage;
		for(uint8_t i = 0; i < MAX_SBUS_CHANNELS; i++) {
			channels[i] = _channels[i];
		}
		_signalLostTimeMs = millis()+_recoveryTimeMs;    
	} else {
		if (millis() <= _signalLostTimeMs) {
			_isValid = true; // still return previous valid frame even if last frames are missing or invalid;
		} else {
			if (prevValid) {
				_errorsCount++;
			}
		}
	}

	interrupts();
	return _isValid;
}

// channels start from 1 to 18
// SBUS has values 0..7FF = 0..2047; Middle is 1023.
// returns value 988..2012 (cleanflight friendly)
uint16_t SBUS::getChannel(uint8_t channel) { // channel is 1-based;
	uint8_t ch = channel-1;
	uint16_t  result = _channels[ch]; 
	return (uint16_t) round((result-SBUS_CENTER)*SBUS_TO_CPPM_CORR+CPPM_CENTER_CORR);
}

uint16_t SBUS::getRawChannel(uint8_t channel) { // channel is 1-based;
	uint8_t ch = channel-1;
	noInterrupts();  // Maybe noInterrupts/interrupts is not needed for 32 bit MCUs...
	uint16_t  result = _channels[ch]; 
	interrupts();
	return result;
}

bool SBUS::signalLossActive() { 
	noInterrupts(); 
	bool result = _signalLossActive; 
	interrupts();
	return result;
}

bool SBUS::failsafeActive() { 
	noInterrupts(); 
	bool result = _failsafeActive; 
	interrupts();
	return result;
}
