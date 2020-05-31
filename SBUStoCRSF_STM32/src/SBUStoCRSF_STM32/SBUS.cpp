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
		_channels[i]      = 0;
	}
	_signalLossActive = false;
	_failsafeActive = false;
	_hasSBUS = false;
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
       _outOfSyncFrames++;
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
      if (_signalLossActive) {
        _signalLossFrames++;
      }
      if (_failsafeActive) {
        _failsafeFrames++;
      }
#ifdef DEBUG_SBUS
			_framesCount++;
#endif    
			gotData = true;
		}
	}

	unsigned long ms = millis();
	if (!gotData) {
		_hasSBUS = (ms <= timeoutMs);	// false if have not received SBUS frame within the timeout;	
	} else {
		timeoutMs = millis()+SBUS_TIMEOUT_MS;
		_hasSBUS = true;
		_gotSBUS = true;
	}

  _isValid = false;
  if (!_gotSBUS)
    return; // return if not ever received valid SBUS frame; is RX disconnected?
    
  bool hasChannels = _hasSBUS && !_signalLossActive && !_failsafeActive; // it might be SL,FS from this frame or previous frame;
  static bool prevHasSBUS = false;
  bool lostSBUS = !_hasSBUS && prevHasSBUS;
  prevHasSBUS = _hasSBUS;
  if (lostSBUS) {
    _timeoutsCount++;    
  }

  bool channelsValid = true;
  if (hasChannels) {
    for(uint8_t i = 0; i < 15; i++) { // Channels 16 and 17 are not checked;
      int x = _channels[i];      
      if (x < SBUS_VALID_MIN || x > SBUS_VALID_MAX) {
        channelsValid = false; 
        if (gotData) { // Increment only for just received frame;
          _invalidChannelsCount++; // Invalid channels errors;
        }
        break;
      }
    }
  }

  if (hasChannels && channelsValid) {
    _missedFrames = 0;
    _isValid = true;
  } else {
    if ((_hasSBUS && _signalLossActive || _failsafeActive) || lostSBUS) // Generic errors: SL or FS or lost SBUS
      _errorsCount++;
    if (_missedFrames < UINT_MAX)
      _missedFrames++;
  }
}

bool SBUS::isAcceptable() {
	if (!_gotSBUS)
		return false;
	if (isValid())
		return true;
	if (hasSBUS() && failsafeActive()) // If RX sent FailSafe, invalidate SBUS signal;
		return false;
  // Not isValid and not FailSafe - maybe no SBUS connection or RX Loss or RX set to No Pulses mode;
  static bool wasMissedBefore = true;
  if (_missedFrames >= _missedFramesLimit) {
    if (!wasMissedBefore) {
      _missedFramesOverflows++;
    }
    wasMissedBefore = true;
    return false;
  }
  wasMissedBefore = false;
	return true;
}

// channels start from 1 to 18
// SBUS has values 0..7FF = 0..2047; Middle is 1023.
// returns value 988..2012 (cleanflight friendly)
uint16_t SBUS::getChannel(uint8_t channel) { // channel is 1-based;
	uint8_t ch = channel-1;
	noInterrupts(); 
	uint16_t  result = _channels[ch]; 
	interrupts();
	return (uint16_t) round((result-SBUS_CENTER)*SBUS_TO_CPPM_CORR+CPPM_CENTER_CORR);
}

uint16_t SBUS::getRawChannel(uint8_t channel) { // channel is 1-based;
	uint8_t ch = channel-1;
	noInterrupts();  // Maybe noInterrupts/interrupts is not needed for 32 bit MCUs...
	uint16_t  result = _channels[ch]; 
	interrupts();
	return result;
}

#ifdef DEBUG_SBUS
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
#endif
