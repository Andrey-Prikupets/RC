#ifndef PXX_h
#define PXX_h

#include <Arduino.h>
#include "debug.h"

#define FAILSAFE_FRAME_EACH 1000

#define PXX_COUNTRY_US         0
#define PXX_COUNTRY_JP         1
#define PXX_COUNTRY_EU         2

#define PXX_PROTO_X16          0
#define PXX_PROTO_D8           1
#define PXX_PROTO_LR12         2

// PXX FLAGS1 fields;
#define PXX_SEND_BIND          1 << 0
#define PXX_COUNTRY_SHIFT      1
#define PXX_COUNTRY_MASK       3
#define PXX_SEND_FAILSAFE      1 << 4
#define PXX_SEND_RANGECHECK    1 << 5
#define PXX_PROTO_SHIFT        6
#define PXX_PROTO_MASK         3

// PXX EXTRA_FLAGS fields;
#define PXX_EXTERNAL_ANTENNA   1 << 0
#define PXX_RX_TELEM_OFF       1 << 1
#define PXX_RX_CHANNEL_9_16    1 << 2
#define PXX_POWER_SHIFT        3
#define PXX_POWER_MASK         3
#define PXX_DISABLE_S_PORT     1 << 5
#define PXX_R9M_EUPLUS         1 << 6 

class PXX_Class
{
    public:
        void begin();
        void send();
        void prepare(int16_t channels[16]); 

        bool getModeBind() { return modeBind; }
        void setModeBind(bool value) { modeBind = value; refreshFlag1(); }
        bool getModeRangeCheck() { return modeRangeCheck; }
        void setModeRangeCheck(bool value) { modeRangeCheck = value; refreshFlag1(); }
        bool getTelemetry() { return telemetry; refreshExtraFlags(); }
        void setTelemetry(bool value) { telemetry = value; }
        bool getSPort() { return sPort; }
        void setSPort(bool value) { sPort = value; refreshExtraFlags(); }
        bool getSend16Ch() { return send16ch; }
        void setSend16Ch(bool value) { send16ch = value; }
        uint8_t getRxNum() { return rx_num; }
        void setRxNum(uint8_t value) { rx_num = value; }
        uint8_t getPower() { return power; }
        void setPower(uint8_t value) { power = value & PXX_POWER_MASK; refreshExtraFlags(); }
        uint8_t getProto() { return proto; }
        void setProto(uint8_t value) { proto = value & PXX_PROTO_MASK; refreshFlag1(); }
        uint8_t getCountry() { return country_code; }
        void setCountry(uint8_t value) { country_code = value & PXX_COUNTRY_MASK; refreshFlag1(); }
        void setEUPlus(bool value) { euPlus = value; refreshExtraFlags(); }
        bool getEUPlus() { return euPlus; }

    private:
        uint8_t  pulses[64];
        int length = 0;
        uint16_t pcmCrc;
        uint32_t pcmOnesCount;
        uint16_t serialByte;
        uint16_t serialBitCount;
        bool     sendUpperChannels;
        uint8_t  flags1;
        uint8_t  extra_flags;

        uint16_t framesCountBeforeFailSafe;
        bool     modeBind;
        bool     modeRangeCheck;
        uint8_t  rx_num;
        uint8_t  power;
        uint8_t  country_code;
        uint8_t  proto;
        bool     telemetry;
        bool     sPort;
        bool     send16ch;
        bool     euPlus; // Flex EU;
        
        void crc(uint8_t data);
        void putPcmSerialBit(uint8_t bit);
        void putPcmPart(uint8_t value);
        void putPcmFlush();
        void putPcmBit(uint8_t bit);
        void putPcmByte(uint8_t byte);
        void putPcmHead();
        uint16_t limit(uint16_t low,uint16_t val, uint16_t high);
        void preparePulses(int16_t channels[8]);
        void refreshFlag1();
        void refreshExtraFlags();
};

extern PXX_Class PXX;

#endif
