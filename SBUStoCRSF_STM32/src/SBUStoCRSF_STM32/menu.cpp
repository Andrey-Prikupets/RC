#include "menu.h"
#include "SBUS.h"
#include <itoa.h>

#ifdef OLED
#include <U8g2lib.h>

// Display selection; Note: after changing display may need to re-design menu;
#ifdef DEBUG // Allocate less display memory in DEBUG mode;
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
  // U8G2_SSD1306_64X48_ER_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#else
  // U8G2_SSD1306_64X48_ER_F_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#endif

uint8_t char_height;
uint8_t char_width;
uint8_t screen_width;
uint8_t screen_height;
#endif

#define MODE_UNDEFINED 0
#define MODE_LOGO 1
#define MODE_SCREENSAVER 2

static byte menuMode = MODE_LOGO;

#define sbus_START 0
#define sbus_LOST 1
#define sbus_OBTAINED 2

static byte sbus_state = sbus_START;
static byte old_sbus_state = sbus_START;

bool sbusModeChanged(void);
extern SBUS sbus;

#ifdef OLED

#ifdef DEBUG_OLED_SPEED
unsigned long oledSpeed;
#endif

// PROGMEM enabled draw functions -----------------------
size_t getLength_F(const __FlashStringHelper *ifsh)
{
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    n++;
  }
  return n;
}

// Moves cursor X position - good for sequential calls;
void drawStr_F(uint8_t x, uint8_t y, const __FlashStringHelper* s_P) {
  u8g.setCursor(x, y);
  u8g.print(s_P);
}

void drawStrRight_F(uint8_t x, uint8_t y, const __FlashStringHelper* s_P) {
  u8g.setCursor((uint8_t) u8g.getWidth()-x-getLength_F(s_P)*(u8g.getStrWidth("W")+1), y);
  u8g.print(s_P);
}

void drawCentered_F(int y, const __FlashStringHelper* s_P) {
  u8g.setCursor((u8g.getWidth()-(getLength_F(s_P)+1)*u8g.getStrWidth("W"))/2, y);     
  u8g.print(s_P);
}
#endif

void menuSetup(void)
{
#ifdef OLED
  u8g.begin();
  u8g.setFont(u8g_font_5x8); // u8g_font_6x10
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDrawColor(1);
  char_height = u8g.getMaxCharHeight(); // u8g.getFontAscent()-u8g.getFontDescent()+1;    
  char_width = u8g.getStrWidth("W")+1;
  screen_width = (uint8_t) u8g.getWidth();
  screen_height = (uint8_t) u8g.getHeight();
#endif  
}

#ifdef OLED
// num = 1..NUM_CHANNELS_SBUS+(number of additional infos);
void setScreenPos(uint8_t num) {
  int x = ((num-1) / 8)*char_width*8;
  int y = ((num-1) % 8)*char_height;
  u8g.setCursor(x, y);
}

// num = 1..NUM_CHANNELS_SBUS;
#define setChannelPos(num) setScreenPos(num)

#define INFO_FAILSAFE      NUM_CHANNELS_SBUS+1
#define INFO_BATTERY       NUM_CHANNELS_SBUS+2
#define INFO_VOLTAGE       NUM_CHANNELS_SBUS+3
#define INFO_RX_STATUS     NUM_CHANNELS_SBUS+4
#define INFO_SBUS_FRAMES   NUM_CHANNELS_SBUS+5
#define INFO_SBUS_ERRORS   NUM_CHANNELS_SBUS+6
#define INFO_SBUS_SL       NUM_CHANNELS_SBUS+7
#define INFO_SBUS_FS       NUM_CHANNELS_SBUS+8
#ifdef DEBUG_OLED_SPEED
#define INFO_OLED_SPEED    NUM_CHANNELS_SBUS+8
#endif

void drawPos_F(uint8_t pos, unsigned int value, const __FlashStringHelper* s_P) {
  char buf[10];
  setScreenPos(pos);
  u8g.print(s_P);
  itoa(value, buf, 10);
  u8g.print(buf);  
}

void drawChannels(void) {
  char buf[10];
  static bool invalidValueFlashing = false;

  if (mTimer1.isTriggered(TIMER_INVALID_FLASHING)) {
    invalidValueFlashing = !invalidValueFlashing;
  }
  
#ifndef DEBUG_SBUS
    int maxChan = NUM_CHANNELS_SBUS;
#else
    int maxChan = 8;
#endif
    for (int i=1; i <= maxChan; ++i) {
      setChannelPos(i);
      int j;
      if (i<10) {
        buf[0]='0';
        j=1;
      } else {
        j=0;
      }
      itoa(i, buf+j, 10);
      buf[2]=':';
#ifndef DEBUG_SBUS
      int16_t x = channels[i-1];
#else
      int16_t x = sbus.getRawChannel(i);
#endif
      if (SBUS::channelValid(x) || invalidValueFlashing) {
        itoa(x, buf+3, 10);
      } else {
        buf[3]=0;
      }
      u8g.print(buf);      
    }  
  
    setScreenPos(INFO_FAILSAFE);
    char* p = buf;
    if (enableCrossfire) {
      *p++ = 'C';
      *p++ = 'R';
      *p++ = 'S';
      *p++ = ' ';      
    }
    if (sbus.isValid()) {
      *p++ = 'O';
      *p++ = 'K';
      *p++ = ' ';
    }
    if (sbus.signalLossActive()) {
      *p++ = 'S';
      *p++ = 'L';
      *p++ = ' ';
    }
    if (sbus.failsafeActive()) {
      *p++ = 'F';
      *p++ = 'S';
    }
    *p++ = 0;
    u8g.print(buf);

#ifdef DEBUG_SBUS
  drawPos_F (8+1, sbus.getInvalidChannelsCount(), F("In:"));
  drawPos_F (8+2, sbus.getMissedFrames(), F("Mi:"));
  drawPos_F (8+3, sbus.getOutOfSyncFrames(), F("Ou:"));
  drawPos_F (8+4, sbus.getTimeoutsCount(), F("Ti:"));
  drawPos_F (8+5, sbus.getBytesCount(), F("By:"));
#endif        
  drawPos_F (INFO_SBUS_FRAMES, sbus.getFramesCount(), F("Fr:"));
  drawPos_F (INFO_SBUS_ERRORS, sbus.getErrorsCount(), F("Er:"));
  drawPos_F (INFO_SBUS_SL, sbus.getSignalLossFrames(), F("SL:"));
#ifdef DEBUG_OLED_SPEED
  drawPos_F (INFO_OLED_SPEED, oledSpeed, F("OL:"));
#else
  drawPos_F (INFO_SBUS_FS, sbus.getFailsafeFrames(),  F("FS:"));
#endif
}

void drawScreenSaver() {
  char buf[10];

  drawChannels();

  setScreenPos(INFO_BATTERY);
  u8g.print(F("Cells:"));      
  itoa(battery1.getNumCells(), buf, 10); // numCells = 1..9;
  buf[1] = 's';
  buf[2] = '\0';
  u8g.print(buf);      

  setScreenPos(INFO_VOLTAGE);
  u8g.print(F("Bat:"));      
  dtostrf(battery1.getCurrVoltage(), 4, 1, buf);
  u8g.print(buf);      

  setScreenPos(INFO_RX_STATUS);
  const __FlashStringHelper* s;
  switch (sbus_state) {
  case sbus_START: s = F("Wait RX");
       break;
  case sbus_OBTAINED: s = F("RX OK");
       break;
  default: s = F("RX Lost"); // sbus_LOST
       break;
  }
  u8g.print(s);      
}

void drawLogo(void) {
  u8g.setDrawColor(1);      
  u8g.drawBox(0, 0, u8g.getWidth(), char_height*2+1);

  u8g.setDrawColor(0);
  drawCentered_F(char_height*0, F(VERSION));   
  drawCentered_F(char_height*1, F("CrossFire relay"));

  u8g.setDrawColor(1);      
  uint8_t y = char_height*2+(char_height/2);
  drawCentered_F(y, F("(c) 2019"));   
  drawCentered_F(y+char_height*1, F("by Andrey"));
  drawCentered_F(y+char_height*2, F("Prikupets"));
}
#endif

void showLogo(void) {
    menuMode = MODE_LOGO;
#ifdef OLED
    u8g.firstPage();
    do  {
      drawLogo();
    } while( u8g.nextPage() );
#endif    
}

void showScreenSaver(void) {
    if (menuMode != MODE_SCREENSAVER) {
        menuMode = MODE_SCREENSAVER;
    }
#ifdef OLED    
#ifdef DEBUG_OLED_SPEED
    unsigned long oledSpeed1 = millis();
#endif
    u8g.firstPage();
    do  {
      drawScreenSaver();
    } while( u8g.nextPage() );  
#ifdef DEBUG_OLED_SPEED
    oledSpeed = millis()-oledSpeed1;
#endif
#endif
}

void setSBUS_Start(void) {
#ifdef DEBUG
  Serial.println("sbus.Start");
#endif
    sbus_state = sbus_START;
    mTimer1.suspend(TIMER_NO_SBUS);
}

void setSBUS_Obtained(void) {
    if (sbus_state == sbus_LOST) {
#ifdef DEBUG
  Serial.println("sbus.Lost->Got");
#endif
      beeper1.stop(&SEQ_MODE_NO_SBUS);
      beeper1.stop(&SEQ_MODE_SBUS_LOST);
      beeper1.play(&SEQ_MODE_GOT_SBUS);
      mTimer1.suspend(TIMER_NO_SBUS);
    }
#ifdef DEBUG
    if (sbus_state == sbus_START) {
  Serial.println("sbus.Start->Got");
    }
#endif
    sbus_state = sbus_OBTAINED;  
}

void setSBUS_Lost(void) {
    if (sbus_state == sbus_OBTAINED) {
#ifdef DEBUG
  Serial.println("sbus.Got->Lost");
#endif
      beeper1.stop(&SEQ_MODE_GOT_SBUS);
      beeper1.play(&SEQ_MODE_SBUS_LOST);
      mTimer1.resume(TIMER_NO_SBUS);
      sbus_state = sbus_LOST;  
    } else
    if (sbus_state == sbus_LOST) {
      if (mTimer1.isTriggered(TIMER_NO_SBUS)) {
#ifdef DEBUG
  Serial.println("sbus.Lost.Seq");
#endif
        beeper1.play(&SEQ_MODE_NO_SBUS);
      }
    }
}

bool sbusModeChanged(void) {
    if (old_sbus_state != sbus_state) {
      old_sbus_state = sbus_state;
      return true;
    }
    return false;
}

void menuLoop(void)
{
  byte refreshMenuMode = MODE_UNDEFINED;

  // Check for additional menu refreshes if something is flashing or displayed data updated; 
  switch(menuMode) {
    case MODE_SCREENSAVER:
      // Refresh Screensaver if CPPM lost/obtained;
      if (sbusModeChanged()) {
          refreshMenuMode = menuMode;
      }
      // Refresh Screensaver with the given update period;
      if (mTimer1.isTriggered(TIMER_SCREEN_UPDATE)) {
          refreshMenuMode = menuMode;
      }
      break;
  }

  // Redraw screens if resfresh required;
  switch(refreshMenuMode) {
    case MODE_SCREENSAVER:
      showScreenSaver();
      break;
  }
}
