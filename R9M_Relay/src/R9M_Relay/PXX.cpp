#include <Arduino.h>
#include "PXX.h"

#define PPM_CENTER                          1500
#define PPM_LOW                             817
#define PPM_HIGH                            2182
#define PPM_HIGH_ADJUSTED                   (PPM_HIGH - PPM_LOW)
#define PXX_CHANNEL_WIDTH                   2048
#define PXX_UPPER_LOW                       2049
#define PXX_UPPER_HIGH                      4094
#define PXX_LOWER_LOW                       1
#define PXX_LOWER_HIGH                      2046

const uint16_t CRC_Short[] =
{
   0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
   0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7
};

uint16_t CRCTable(uint8_t val)
{
  return CRC_Short[val&0x0F] ^ (0x1081 * (val>>4));
}


void PXX_Class::crc( uint8_t data )
{
    pcmCrc=(pcmCrc<<8) ^ CRCTable((pcmCrc>>8)^data) ;
}

#ifndef DEBUG
void USART_Init(long baud)
{
    int _baud = (16000000 / (2 * baud)) - 1;

    UBRR0 = 0;

    /* Setting the XCKn port pin as output, enables master mode. */
    //XCKn_DDR |= (1<<XCKn);
    //DDRD = DDRD | B00000010;

    /* Set MSPI mode of operation and SPI data mode 0. */
    UCSR0C = (1<<UMSEL01)|(1<<UMSEL00)|(1<<UDORD0);

    /* Enable receiver and transmitter. */
    UCSR0B = (1<<TXEN0);
    /* Set baud rate. */
    /* IMPORTANT: The Baud Rate must be set after the transmitter is enabled */
    UBRR0 = _baud;
}

void USART_Send(uint8_t data) {
    /* Put data into buffer, sends the data */
    UDR0 = data;

    //Wait for the buffer to be empty
    while ( !( UCSR0A & (1<<UDRE0)) );
}
#endif

void PXX_Class::begin()
{
#ifndef DEBUG
// 125000; 0=8; 1=8/16; SUM = 16/24;
// 133333; 0=7.5, 1=7.5/15
// 150000; 0=6.6, 1=6.6/13.25;
// Hourus; 0=10.1,1=5.87/13.84; SUM=15.8/23.84;
  USART_Init(125000); // 125000
#else
  Serial.println(F("PXX.begin"));
#endif  
  framesCountBeforeFailSafe = 0;
  refreshFlag1();
  refreshExtraFlags();
  initialized = true;
}

void PXX_Class::end()
{
  initialized = false;
}

void PXX_Class::putPcmSerialBit(uint8_t bit)
{
    serialByte >>= 1;
    if (bit & 1)
    {
        serialByte |= 0x80;
    }

    if (++serialBitCount >= 8)
    {
        pulses[length++] = serialByte;
        serialBitCount = 0;
    }
}

// 8uS/bit 01 = 0, 001 = 1
// AP; 8uS/bit 01 = 0, 011 = 1
void PXX_Class::putPcmPart(uint8_t value)
{
    putPcmSerialBit(0);

    if (value)
    {
        putPcmSerialBit(1); // AP; was 0;
    }

    putPcmSerialBit(1);
}

void PXX_Class::putPcmFlush()
{
    while (serialBitCount != 0)
    {
        putPcmSerialBit(1);
    }
}

void PXX_Class::putPcmBit(uint8_t bit)
{
    if (bit)
    {
        pcmOnesCount += 1;
        putPcmPart(1); 
    }
    else
    {
        pcmOnesCount = 0;
        putPcmPart(0);
    }

    if (pcmOnesCount >= 5)
    {
        putPcmBit(0);                                // Stuff a 0 bit in
    }
}

void PXX_Class::putPcmByte(uint8_t byte)
{
    crc(byte);

    for (uint8_t i=0; i<8; i++)
    {
        putPcmBit(byte & 0x80);
        byte <<= 1;
    }
}

void PXX_Class::putPcmHead()
{
    // send 7E, do not CRC
    // 01111110
    putPcmPart(0);
    putPcmPart(1);
    putPcmPart(1);
    putPcmPart(1);
    putPcmPart(1);
    putPcmPart(1);
    putPcmPart(1);
    putPcmPart(0);
}

#ifdef SIMULATE_CHANNELS
  static int16_t channels_sim[16] = {1400, 1200, 1450, 1550,  1600, 1700, 1800, 1900,  
                                     2000, 1950, 1850, 1750,  1650, 1550, 1450, 1350}; // AP; 
#endif

void PXX_Class::prepare(int16_t channels[16])
{
    uint16_t chan=0, chan_low=0;

    length = 0;
    pcmCrc = 0;
    pcmOnesCount = 0;

    /* Preamble */
    // AP; Temporary removed to follow Horus protocol output;
    /*
    putPcmPart(0);
    putPcmPart(0);
    putPcmPart(0);
    putPcmPart(0);
    */ 

    // putPcmPart(0); // AP; added because 1st 0 byte was not output by some reason;

    /* Sync */
    putPcmHead();

    // Rx Number
    putPcmByte(rx_num);

    framesCountBeforeFailSafe++;
    bool failsafe = framesCountBeforeFailSafe >= FAILSAFE_FRAME_EACH;
    if (failsafe) {
      if (!send16ch || framesCountBeforeFailSafe >= FAILSAFE_FRAME_EACH+1) { // 1 extra frame in 16ch mode;
        framesCountBeforeFailSafe = 0;
      }
    }
    putPcmByte(flags1 | (failsafe ? PXX_SEND_FAILSAFE : 0)); 

    // FLAG2
    putPcmByte(0);

    if (failsafe) {
#ifdef BEEP_FAILSAFE      
        beeper1.play(&SEQ_FAILSAFE);
#endif
        // Send No Pulses failsafe setting;
        if (!sendUpperChannels) { // all channels 1-8 have 0;
          for (int i=0; i<12; i++) {
            putPcmByte(0);
          }
        } else {
          for (int i=0; i<4; i++) { // all channels 9-16 have 2048;
            putPcmByte(0);
            putPcmByte(1 << 3);
            putPcmByte(1 << 7);
          }              
        }
    } else {
      // PPM
      for (int i=0; i<8; i++)
      {
#ifdef SIMULATE_CHANNELS
          int channelPPM = channels_sim[(sendUpperChannels ? (8 + i) : i)];
#else
          int channelPPM = channels[(sendUpperChannels ? (8 + i) : i)];
#endif
          float convertedChan = ((float(channelPPM) - float(PPM_LOW)) / (float(PPM_HIGH_ADJUSTED))) * float(PXX_CHANNEL_WIDTH);
          chan = limit((sendUpperChannels ? PXX_UPPER_LOW : PXX_LOWER_LOW),
                       convertedChan,
                       (sendUpperChannels ? PXX_UPPER_HIGH : PXX_LOWER_HIGH));
  
          if (i & 1)
          {
              putPcmByte(chan_low); // Low byte of channel
              putPcmByte(((chan_low >> 8) & 0x0F) | (chan << 4));  // 4 bits each from 2 channels
              putPcmByte(chan >> 4);  // High byte of channel
          }
          else
          {
              chan_low = chan;
          }
      }
    }

    // CRC16
    putPcmByte(extra_flags);
    chan = pcmCrc;
    putPcmByte(chan>>8);
    putPcmByte(chan);

    // Sync
    putPcmHead();
    putPcmFlush();
}

void PXX_Class::refreshFlag1() {
    flags1 = (modeBind ? PXX_SEND_BIND | (country_code << PXX_COUNTRY_SHIFT) : 0) | // send country_code only during bind process;
             (modeRangeCheck ? PXX_SEND_RANGECHECK : 0) |
              proto << PXX_PROTO_SHIFT;
}

void PXX_Class::refreshExtraFlags() {
    extra_flags = (telemetry ? 0 : PXX_RX_TELEM_OFF) |
                   power << PXX_POWER_SHIFT | 
                  (sPort ? 0 : PXX_DISABLE_S_PORT) |
                  (euPlus ? PXX_R9M_EUPLUS : 0);
}

void PXX_Class::send()
{
    if (initialized) {
      for(int i = 0; i < length; i++)
      {
#ifndef DEBUG
        USART_Send(pulses[i]);
#endif        
      }
    }
    sendUpperChannels = send16ch ? !sendUpperChannels : 0;
}

uint16_t PXX_Class::limit(uint16_t low, uint16_t val, uint16_t high) {
    if(val < low)
    {
        return low;
    }

    if(val > high)
    {
        return high;
    }

    return val;
}

PXX_Class PXX;
