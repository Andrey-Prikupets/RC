#include <string.h>
#include "menu.h"
#include "PXX.h"
#include "CPPM.h"
#include "VRx_Rssi.h"

#include <EEPROM.h>

#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))

#ifdef OLED
//#include "U8glib.h"
#include <U8g2lib.h>
//#include <U8x8lib.h>

// Display selection; Note: after changing display may need to re-design menu;
//U8GLIB_SSD1306_64X48 u8g(U8G_I2C_OPT_NONE || U8G_I2C_OPT_NO_ACK || U8G_I2C_OPT_FAST);  // I2C / TWI 
#ifdef DEBUG // Allocate less display memory in DEBUG mode;
  U8G2_SSD1306_64X48_ER_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#else
  U8G2_SSD1306_64X48_ER_F_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#endif

uint8_t menu_current = 0;

boolean menuItemFlashing = false;

#endif // #ifdef OLED

#ifndef OLED

// SeralCommand -> https://github.com/kroimon/Arduino-SerialCommand.git
#include <SerialCommand.h>

SerialCommand sCmd; // Create Serial Command Object.
static bool cliActive = false;

#endif // #ifndef OLED

// Settings version;
const uint8_t CURRENT_VERSION = 0x12;

#ifdef OLED

#define MODE_UNDEFINED 0
#define MODE_LOGO 1
#define MODE_SCREENSAVER 2
#define MODE_RADIO 3
#define MODE_MENU 4
#define MODE_MENU 5
#define MODE_CHANNELS 6

#ifdef RC_MIN_MAX
#define MODE_CHANNELS_MIN_MAX 7
#endif

#ifdef RELAY
#define MODE_RELAY 8
#endif

static byte menuMode = MODE_LOGO;

// Private variables;
#define KEY_NONE 0
#define KEY_NEXT 1
#define KEY_SELECT 2

#endif // #ifdef OLED

#define CPPM_START 0
#define CPPM_LOST 1
#define CPPM_OBTAINED 2

static byte cppm_state = CPPM_START;
static byte old_cppm_state = CPPM_START;

bool cppmModeChanged(void);

#define RADIO_US_FCC  0 
#define RADIO_EU_LBT  1
#define RADIO_EU_FLEX 2
byte radioMode;

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

#ifdef OLED

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

// Keyboard handling -----------------------------------
uint8_t peek(void) {
  if (digitalRead(PIN_KEY_NEXT) == LOW)
    return KEY_NEXT;
  else if (digitalRead(PIN_KEY_SELECT) == LOW)
    return KEY_SELECT;
  else 
    return KEY_NONE;  
}

uint8_t handleKeys(void) {
  static uint8_t key = KEY_NONE;
  static unsigned long pressMills=0;
  static uint8_t keyPrev = KEY_NONE;
  uint8_t c = peek();
  
  if (c != keyPrev) {
    keyPrev = c;
    pressMills = millis()+BOUNCE_TICK;  
  } else {
    if (millis() >= pressMills) {
      key = c;
    }    
  }
  return key;
}

#define MENU_LEFT 0
#define MENU_TOP 0

void drawMenuItem(uint8_t i, const __FlashStringHelper* s_P, const char * s1) {
    uint8_t y;
    uint8_t screen_width = (uint8_t) u8g.getWidth();
    uint8_t char_height = u8g.getFontAscent()-u8g.getFontDescent();
    y = char_height*i+MENU_TOP;
    u8g.setDrawColor(1);      
    if ( i == menu_current ) {
      u8g.drawBox(MENU_LEFT, y-1, screen_width-MENU_LEFT, char_height+1);
      u8g.setDrawColor(0);
    }
    if (s_P) {
      u8g.setCursor(MENU_LEFT+1, y-1);
      u8g.print(s_P);
    }
    if (s1) {
      u8g.drawStr(screen_width-u8g.getStrWidth(s1), y-1, s1);      
    }
}

#endif // #ifdef OLED

const __FlashStringHelper* getPowerStr(void) {
  if (radioMode != RADIO_EU_LBT) { // RADIO_US_FCC || RADIO_EU_FLEX
    switch (PXX.getPower()) {
  case 0: return F("10 mW");
       break;
  case 1: return F("100 mW");
       break;
  case 2: return F("500 mW");
       break;
  case 3: return F("Auto < 1 W");
       break;
    }
  } else {
    switch (PXX.getPower()) {
  case 0: return F("25 mW 8ch");
       break;
  case 1: return F("25 mW");
       break;
  case 2: return F("200 mW");
       break;
  case 3: return F("500 mW");
       break;
    }    
  }
  return NULL;
}

const __FlashStringHelper* getBandStr(void) {
  switch(radioMode) {
    case RADIO_US_FCC: return F("US FCC 915MHz");
    case RADIO_EU_LBT: return F("EU LBT 868MHz");
    default:           return F("FLEX   868MHz"); // RADIO_EU_FLEX
  }
}

const __FlashStringHelper* getBandStrShort(void) {
  switch(radioMode) {
    case RADIO_US_FCC: return F("US 915");
    case RADIO_EU_LBT: return F("EU 868");
    default:           return F("FLEX 868"); // RADIO_EU_FLEX
  }
}

#ifdef OLED

void prepareForDrawing(bool big_font) {
  u8g.setDrawColor(1);      
  u8g.setFont(big_font ? u8g_font_6x10 : u8g_font_5x8); 
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
}

void drawMenu(void) {
  prepareForDrawing(true);

#ifdef DEBUG
  Serial.print("drawMenu="); Serial.println(menu_current);
#endif  
  int8_t i = 0;
  drawMenuItem(i, F("Radio Set."), NULL); i++;
  drawMenuItem(i, F("RX Monitor"), NULL); i++;
#ifdef RC_MIN_MAX
  drawMenuItem(i, F("RX Min/Max"), NULL); i++;
#endif
#ifdef RELAY
  drawMenuItem(i, F("Relay Set."), NULL);
#endif
}

static bool invalidValueFlashing = false;

#ifdef RC_MIN_MAX
static bool showChannels_Min = false;
#endif

int16_t getChannel_Raw_Min_Max(uint8_t channel) {
  int16_t* arr = 
#ifdef RC_MIN_MAX
    menuMode == MODE_CHANNELS ? channels : showChannels_Min ? channelsMin : channelsMax;
#else
    channels;
#endif
  return arr[channel];
}

void drawChannelItems(uint8_t i, uint8_t channelLeft, uint8_t channelRight) {
  uint8_t screen_width = (uint8_t) u8g.getWidth();
  uint8_t char_height = u8g.getFontAscent()-u8g.getFontDescent()+1;
  uint8_t char_width = u8g.getStrWidth("W");
  char buf[7];
  int y = MENU_TOP+char_height*i;

  u8g.setDrawColor(1);      
  u8g.drawBox(MENU_LEFT, y-1, char_width+1, char_height+1);
  u8g.drawBox(MENU_LEFT+(screen_width+char_width)/2-1, y-1, char_width+2, char_height+1);

  u8g.setDrawColor(0);
  itoa(channelLeft+1, buf, 10);
  u8g.drawStr(MENU_LEFT, y, buf);

  itoa(channelRight+1, buf, 10);
  u8g.drawStr(MENU_LEFT+(screen_width+char_width)/2, y, buf); 

  u8g.setDrawColor(1);
  if (menuMode == MODE_CHANNELS
#ifdef RC_MIN_MAX  
  || channelsMinMaxSet
#endif
  ) {
    int16_t x = getChannel_Raw_Min_Max(channelLeft);
    if (
#ifdef RC_MIN_MAX
      menuMode == MODE_CHANNELS_MIN_MAX || 
#endif
      (channelValid(x) || invalidValueFlashing)) {
      itoa(x, buf, 10);
      u8g.drawStr(MENU_LEFT+char_width+2, y, buf); 
    }

    x = getChannel_Raw_Min_Max(channelRight);
    if (
#ifdef RC_MIN_MAX
      menuMode == MODE_CHANNELS_MIN_MAX || 
#endif
      (channelValid(x) || invalidValueFlashing)) {
      itoa(x, buf, 10);
      u8g.drawStr(screen_width-u8g.getStrWidth(buf), y, buf);
    }
  }
}

void drawChannels(void) {
  prepareForDrawing(true);

  if (timer1.isTriggered(TIMER_INVALID_FLASHING)) {
    invalidValueFlashing = !invalidValueFlashing;
  }
  
#ifdef RC_MIN_MAX
  if (timer1.isTriggered(TIMER_CHANNELS_MIN_MAX_FLIP)) {
    showChannels_Min = !showChannels_Min;
  }
#endif
  
  if (menuMode == MODE_CHANNELS && !cppmActive) {
    drawStr_F(MENU_LEFT, MENU_TOP, F("No CPPM")); 
    // Errs: NNN
    // Fail: 1
  } else {
    // CH 1 | CH 2
    // CH 3 | CH 4
    // CH 5 | CH 6
    // CH 7 | CH 8
    // Errs: NNN
    // Fail: 1
    drawChannelItems(0, 0, 1);
    drawChannelItems(1, 2, 3);
    drawChannelItems(2, 4, 5);
    drawChannelItems(3, 6, 7);
  }

  uint8_t screen_width = (uint8_t) u8g.getWidth();
  uint8_t char_height = u8g.getFontAscent()-u8g.getFontDescent()+1;
  uint8_t char_width = u8g.getStrWidth("W");

  int y = MENU_TOP+char_height*4;
  u8g.drawStr(MENU_LEFT, y, "ER"); 
  char buf[5];
  itoa(CPPM.getErrorCount(), buf, 10);
  u8g.drawStr(MENU_LEFT+char_width*2+4, y, buf);

  u8g.drawStr(MENU_LEFT+(screen_width+char_width)/2, y, 
#ifdef RC_MIN_MAX
    menuMode == MODE_CHANNELS ? "=" : showChannels_Min ? "<" : ">"
#else
    "="
#endif
    ); 

  itoa(CPPM.getFailReason(), buf, 10);
  int x = screen_width-u8g.getStrWidth(buf);
  u8g.drawStr(x, y, buf);      
  u8g.drawStr(x-char_width*2-3, y, "FL"); 
}

void drawMenuRadio(void) {
  prepareForDrawing(false);
  drawMenuItem(0, getBandStr(), NULL);

  char buf[5];

  itoa(PXX.getPower(), buf, 10);
  drawMenuItem(1, getPowerStr(), buf);
  
  itoa(PXX.getRxNum(), buf, 10);
  drawMenuItem(2, F("Receiver"), buf);
  
  drawMenuItem(3, F("TELEM "), PXX.getTelemetry() ? "Yes" : "No");
  drawMenuItem(4, F("S-Port "), PXX.getSPort() ? "Yes" : "No");
  drawMenuItem(5, !PXX.getModeRangeCheck() || menuItemFlashing ? F(">RANGE CHECK") : NULL, NULL);
  drawMenuItem(6, !PXX.getModeBind() || menuItemFlashing ? F(">BIND") : NULL, NULL);  
}

void drawScreenSaver() {
  char buf[5];

  prepareForDrawing(true);

  uint8_t screen_width = (uint8_t) u8g.getWidth();
  uint8_t char_height = u8g.getFontAscent()-u8g.getFontDescent();
  int y = MENU_TOP;
  drawStr_F(MENU_LEFT, y, F("Bat:"));      
  itoa(battery1.getNumCells(), buf, 10); // numCells = 1..9;
  buf[1] = 's';
  buf[2] = '\0';
  u8g.print(buf);      

  dtostrf(battery1.getCurrVoltage(), 4, 1, buf);
  u8g.drawStr(screen_width-u8g.getStrWidth(buf), y, buf);
  y += char_height;

  const __FlashStringHelper* s;
  switch (cppm_state) {
  case CPPM_START: s = F("RX: Wait");
       break;
  case CPPM_OBTAINED: s = F("RX: OK");
       break;
  default: s = F("RX: Lost"); // CPPM_LOST
       break;
  }
  drawStr_F(MENU_LEFT, y, s); 
  if (CPPM.getFailReason()) {
    itoa(CPPM.getFailReason(), buf, 10);
    u8g.drawStr(screen_width-u8g.getStrWidth(buf), y, buf);          
  }
  y += char_height;
  
  drawStr_F(MENU_LEFT, y, getBandStrShort()); 
  y += char_height;

  drawStr_F(MENU_LEFT, y, getPowerStr()); 

#ifdef RELAY
  y += char_height;
  drawStr_F(MENU_LEFT, y, F("Relay:"));   
  switch (getRelayActive()) {
  case RELAY_ACTIVE_PXX: s = F("PXX");
       break;
  case RELAY_ACTIVE_CPPM: s = F("PPM");
       break;
  default: s = F("---");
       break;
  }
  drawStrRight_F(0, y, s);
#endif
}

void drawLogo(void) {
  prepareForDrawing(true);

  uint8_t char_height = u8g.getFontAscent()-u8g.getFontDescent();

  u8g.setDrawColor(1);      
  u8g.drawBox(MENU_LEFT, MENU_TOP, u8g.getWidth()-MENU_LEFT, char_height*2+1);

  u8g.setDrawColor(0);
  drawCentered_F(MENU_TOP+char_height*0, F(VERSION));   
  drawCentered_F(MENU_TOP+char_height*1, F("R9M relay"));

  u8g.setDrawColor(1);      
  uint8_t y = MENU_TOP+char_height*2+(MENU_TOP+char_height/2);
  drawCentered_F(y, F("(c) 2019"));   
  drawCentered_F(y+char_height*1, F("by Andrey"));
  drawCentered_F(y+char_height*2, F("Prikupets"));
}

#ifdef RELAY
void drawRelay() {
  char buf[7];

  prepareForDrawing(false);

  uint8_t screen_width = (uint8_t) u8g.getWidth();
  uint8_t char_height = u8g.getFontAscent()-u8g.getFontDescent()+1;
  uint8_t char_width = u8g.getStrWidth("W");
  int y = MENU_TOP;
  int x;

  buf[0] = 'C';
  buf[1] = 'H';
  itoa(relayChannel+1, buf+2, 10);
  drawStr_F(MENU_LEFT, y, F("Relay"));   
  u8g.drawStr(screen_width-u8g.getStrWidth(buf), y, buf);
  y += char_height;

  itoa(gpsModeChannel+1, buf+2, 10);
  drawStr_F(MENU_LEFT, y, F("GPS"));   
  u8g.drawStr(screen_width-u8g.getStrWidth(buf), y, buf);
  y += char_height;

  itoa(gpsHoldValue, buf, 10);
  drawStr_F(MENU_LEFT, y, F("GPSHOLD"));   
  u8g.drawStr(screen_width-u8g.getStrWidth(buf), y, buf);
  y += char_height;

  // Print: PXX: 1111-2222
  drawStr_F(MENU_LEFT, y, F("PXX:"));
  u8g.print(itoa(activePXX_Min, buf, 10));  
  u8g.print("-");
  u8g.print(itoa(activePXX_Max, buf, 10));

  y += char_height;
  
  // Print: CPPM: 1111-2222
  drawStr_F(MENU_LEFT, y, F("PPM:"));
  u8g.print(itoa(activeCPPM_Min, buf, 10));
  u8g.print("-");
  u8g.print(itoa(activeCPPM_Max, buf, 10));
}
#endif // #ifdef RELAY

void write_settings(void);
void adjust_settings(void);
void adjust_timer(void);

#define MENU_ITEMS_RADIO 7

void updateMenuRadio(uint8_t key) {  
  switch ( key ) {
    case KEY_NEXT:
      beeper1.play(&SEQ_KEY_NEXT);
      menu_current = menu_current >= MENU_ITEMS_RADIO-1 ? 0 : menu_current+1;
      PXX.setModeRangeCheck(false); // Turn off BIND and RANGECHECK if moved in menu;
      PXX.setModeBind(false);
      adjust_timer();
      break;
    case KEY_SELECT:
      beeper1.play(&SEQ_KEY_SELECT);
      switch(menu_current) {
        case 0: // radioMode = radioMode >= RADIO_EU_FLEX ? RADIO_US_FCC : radioMode++;
                radioMode = radioMode == RADIO_US_FCC ? RADIO_EU_LBT : radioMode == RADIO_EU_LBT ? RADIO_EU_FLEX : RADIO_US_FCC;
                adjust_settings(); break;
        case 1: PXX.setPower(PXX.getPower() >= 3 ? 0 : PXX.getPower()+1); 
                adjust_settings(); break;
        case 2: PXX.setRxNum(PXX.getRxNum() >= MAX_RX_NUM ? 0 : PXX.getRxNum()+1); break;
        case 3: PXX.setTelemetry(!PXX.getTelemetry()); break;
        case 4: PXX.setSPort(!PXX.getSPort()); break;
        case 5: PXX.setModeRangeCheck(!PXX.getModeRangeCheck()); 
                adjust_timer(); break;
        case 6: PXX.setModeBind(!PXX.getModeBind()); 
                adjust_timer(); break;
      }
      if (menu_current <= 4) {
        write_settings();
      }
      break;
  }
}

#ifdef RC_MIN_MAX
  #ifdef RELAY
    #define MENU_ITEMS 4
  #else
    #define MENU_ITEMS 3
  #endif
#else
  #ifdef RELAY
    #define MENU_ITEMS 3
  #else
    #define MENU_ITEMS 2
  #endif
#endif

uint8_t updateMenu(uint8_t key) {  
  switch ( key ) {
    case KEY_NEXT:
      beeper1.play(&SEQ_KEY_NEXT);
      menu_current = menu_current >= MENU_ITEMS-1 ? 0 : menu_current+1;
#ifdef DEBUG
  Serial.print("updateMenu="); Serial.println(menu_current);
#endif  
      break;
    case KEY_SELECT:
      beeper1.play(&SEQ_KEY_SELECT);
      switch(menu_current) {
        case 0: return MODE_RADIO;
        case 1: return MODE_CHANNELS;
#ifdef RC_MIN_MAX
        case 2: return MODE_CHANNELS_MIN_MAX;
  #ifdef RELAY
        case 3: return MODE_RELAY;
  #endif
#else
  #ifdef RELAY
        case 2: return MODE_RELAY;
  #endif
#endif
      }
      break;
  }
  return MODE_UNDEFINED;
}

void adjust_timer(void) {
  if (PXX.getModeRangeCheck() || PXX.getModeBind()) {
    timer1.resume(TIMER_MENU_FLASHING);
    timer1.suspend(TIMER_SCREENSAVER);
  } else {
    timer1.suspend(TIMER_MENU_FLASHING);  
    timer1.resume(TIMER_SCREENSAVER);
  }  
}

bool update_menu_flash_timer(void) {
  if (timer1.isTriggered(TIMER_MENU_FLASHING)) {
    menuItemFlashing = !menuItemFlashing;  
    return true;
  }
  return false;
}

#endif // #ifdef OLED

void adjust_settings(void) {
  switch (radioMode) {
    case RADIO_US_FCC: PXX.setCountry(PXX_COUNTRY_US); PXX.setEUPlus(false); break;   
    case RADIO_EU_LBT: PXX.setCountry(PXX_COUNTRY_EU); PXX.setEUPlus(false); break;   
    default:           PXX.setCountry(PXX_COUNTRY_EU); PXX.setEUPlus(true); // RADIO_EU_FLEX
  }
  bool send8Ch = (radioMode == RADIO_EU_LBT) && (PXX.getPower() == 0); // 25mW 8Ch;
  PXX.setSend16Ch (!send8Ch);
}

void read_settings(void) {
  uint8_t b;
  b = EEPROM.read(0);
  if (b != CURRENT_VERSION)
    return;

  radioMode = EEPROM.read(1);
  b = EEPROM.read(2);
  PXX.setPower(b > 3 ? 0 : b);
  adjust_settings();  

  b = EEPROM.read(3);
  PXX.setRxNum(b > MAX_RX_NUM ? 0 : b);

  PXX.setTelemetry(EEPROM.read(4) != 0);
  PXX.setSPort(EEPROM.read(5) != 0);

#ifdef RELAY
  relayEnabled = EEPROM.read(6) != 0;
  EEPROM.get(7, relayChannel);
  EEPROM.get(8, gpsModeChannel);
  EEPROM.get(9, gpsHoldValue);
  EEPROM.get(11, activePXX_Min);
  EEPROM.get(13, activePXX_Max);
  EEPROM.get(15, activeCPPM_Min);
  EEPROM.get(17, activeCPPM_Max);
#endif

#ifdef SHOW_RSSI
  EEPROM.get(19, vrx_rssi_min);
  EEPROM.get(21, vrx_rssi_max);  
#endif

#ifdef RELAY
  holdThrottleEnabled = EEPROM.read(23) != 0;
  EEPROM.get(24, midThrottle);
  EEPROM.get(26, minThrottle);
  EEPROM.get(28, armCPPMChannel);
  EEPROM.get(30, armPXXChannel);
  EEPROM.get(32, armCPPM_Min);
  EEPROM.get(34, armCPPM_Max);
  EEPROM.get(36, armPXX_Min);
  EEPROM.get(38, armPXX_Max);
  EEPROM.get(40, safeThrottle);
#endif
}

void init_settings(void) {
  PXX.setModeBind(false);
  PXX.setModeRangeCheck(false);

  PXX.setPower(0);
  radioMode = RADIO_US_FCC;
  adjust_settings(); // setCountry, setEUPlus;
  
  PXX.setRxNum(0);
  PXX.setTelemetry(true);
  PXX.setSPort(true);
  PXX.setProto(PXX_PROTO_X16);

#ifdef RELAY
  relayEnabled = RELAY_ENABLED;
  relayChannel = RELAY_CHANNEL;      // Channel to switch between PXX and PPM control; Allowed only channels CH5..CH8;
  gpsModeChannel = GPS_MODE_CHANNEL; // Set it to channel to enable GPS HOLD mode; Allowed only channels CH5..CH8;
  gpsHoldValue = GPS_HOLD_VALUE;    // Set it to enable GPS HOLD in GPS_MODE_CHANNEL on both relay and mission drone;

// RELAY_CHANNEL signal boundaries to enable PXX or CPPM control;
  activePXX_Min = ACTIVE_PXX_MIN;   // Min. value for make PXX control active;
  activePXX_Max = ACTIVE_PXX_MAX;   // Max. value for make PXX control active;
  activeCPPM_Min = ACTIVE_CPPM_MIN; // Min. value for make CPPM control active;
  activeCPPM_Max = ACTIVE_CPPM_MAX; // Max. value for make CPPM control active;

  holdThrottleEnabled = HOLD_THROTTLE_ENABLED; // Enable setting mid throttle (normally, 1500) for armed inactive copter and min throttle for disarmed inactive copter;
  midThrottle = MID_THROTTLE;       // Mid throttle value;
  minThrottle = MIN_THROTTLE;       // Min throttle value;
  safeThrottle = SAFE_THROTTLE;     // Minimum safe throttle value when copter considered flying;

// ARM channel signal boundaries for PXX or CPPM control; only armed copter will receive mid throttle when inactive; not armed will receive min throtlle;
  armCPPMChannel = ARM_CPPM_CHANNEL; // Set it to channel to arm CPPM controlled copter; Allowed only channels CH5..CH8;
  armPXXChannel  = ARM_PXX_CHANNEL;  // Set it to channel to arm PXX controlled copter; Allowed only channels CH5..CH8;
  armCPPM_Min    = ARM_CPPM_MIN;     // Min. value for make CPPM arm;
  armCPPM_Max    = ARM_CPPM_MAX;     // Max. value for make CPPM arm;
  armPXX_Min     = ARM_PXX_MIN;      // Min. value for make PXX arm;
  armPXX_Max     = ARM_PXX_MAX;      // Max. value for make PXX arm;
#endif  

#ifdef SHOW_RSSI
  vrx_rssi_min = VRX_RSSI_MIN;
  vrx_rssi_max = VRX_RSSI_MAX;
#endif
}

void write_settings(void) {
  EEPROM.update(0, CURRENT_VERSION);
  EEPROM.update(1, radioMode);
  EEPROM.update(2, PXX.getPower());
  EEPROM.update(3, PXX.getRxNum());
  EEPROM.update(4, PXX.getTelemetry() ? 1 : 0);
  EEPROM.update(5, PXX.getSPort() ? 1 : 0);

#ifdef RELAY
  EEPROM.update(6, relayEnabled ? 1 : 0);
  EEPROM.put(7, relayChannel);
  EEPROM.put(8, gpsModeChannel);
  EEPROM.put(9, gpsHoldValue);
  EEPROM.put(11, activePXX_Min);
  EEPROM.put(13, activePXX_Max);
  EEPROM.put(15, activeCPPM_Min);
  EEPROM.put(17, activeCPPM_Max);
#endif  

#ifdef SHOW_RSSI
  EEPROM.put(19, vrx_rssi_min);
  EEPROM.put(21, vrx_rssi_max);  
#endif

#ifdef RELAY
  EEPROM.update(23, holdThrottleEnabled ? 1 : 0);
  EEPROM.put(24, midThrottle);
  EEPROM.put(26, minThrottle);
  EEPROM.put(28, armCPPMChannel);
  EEPROM.put(30, armPXXChannel);
  EEPROM.put(32, armCPPM_Min);
  EEPROM.put(34, armCPPM_Max);
  EEPROM.put(36, armPXX_Min);
  EEPROM.put(38, armPXX_Max);
  EEPROM.put(40, safeThrottle);
#endif  

}

#ifndef OLED

void init_commands(void);

#endif // #ifndef OLED

void menuSetup(void)
{
#ifdef OLED
  u8g.begin();
  pinMode(PIN_KEY_NEXT, INPUT_PULLUP);           // set pin to input with pullup
  pinMode(PIN_KEY_SELECT, INPUT_PULLUP);         // set pin to input with pullup
#else
  pinMode(PIN_JUMPER_SETUP, INPUT_PULLUP);       // set pin to input with pullup
  init_commands();
#endif 
  init_settings();
  read_settings();
#ifdef OLED
  adjust_timer();
#endif
}

#ifdef OLED
void showLogo(void) {
    menuMode = MODE_LOGO;
    u8g.firstPage();
    do  {
      drawLogo();
    } while( u8g.nextPage() );
}

void showScreenSaver(void) {
    if (menuMode != MODE_SCREENSAVER) {
        timer1.suspend(TIMER_SCREENSAVER);
        menuMode = MODE_SCREENSAVER;
    }
    u8g.firstPage();
    do  {
      drawScreenSaver();
    } while( u8g.nextPage() );  
}

void showMenuRadio() {
    if (menuMode != MODE_RADIO) {
        timer1.resume(TIMER_SCREENSAVER);
        menuMode = MODE_RADIO;
    }
    u8g.firstPage();
    do  {
      drawMenuRadio();
    } while( u8g.nextPage() );  
}

void showMenu() {
    if (menuMode != MODE_MENU) {
        timer1.resume(TIMER_SCREENSAVER);
        menuMode = MODE_MENU;
    }
    u8g.firstPage();
    do  {
      drawMenu();
    } while( u8g.nextPage() );  
}

void showChannels(uint8_t mode) {
    if (menuMode != mode) {
        timer1.suspend(TIMER_SCREENSAVER);
        menuMode = mode;
    }
    u8g.firstPage();
    do  {
      drawChannels();
    } while( u8g.nextPage() );  
}

#ifdef RELAY
void showRelay() {
    if (menuMode != MODE_RELAY) {
        menuMode = MODE_RELAY;
    }
    u8g.firstPage();
    do  {
      drawRelay();
    } while( u8g.nextPage() );  
}
#endif

#endif // #ifdef OLED

void handleBeeps(void) {
  if (timer1.isTriggered(TIMER_MODE_SOUND)) {
    if (PXX.getModeRangeCheck()) {
      beeper1.play(&SEQ_MODE_RANGE_CHECK);
    } else
    if (PXX.getModeBind()) {
      beeper1.play(&SEQ_MODE_BIND);
    }
  }
}

#ifndef OLED
void handlePeriodic();
#endif

void menuLoop(void)
{
#ifdef OLED
  byte refreshMenuMode = MODE_UNDEFINED;

  static uint8_t last_key_code = KEY_NONE;
  uint8_t key = handleKeys();
  if (key != KEY_NONE && last_key_code != key) {
    timer1.resetNotSuspended(TIMER_SCREENSAVER); // Reset non-suspended Screensaver timer if any key pressed;
    if (menuMode == MODE_MENU) { // If key is pressed in Main menu, get the new mode; if submenu selected, reset menu line;
        refreshMenuMode = updateMenu(key);
        if (refreshMenuMode != MODE_UNDEFINED) {
#ifdef DEBUG
  Serial.print("menuLoop1=0"); Serial.println(refreshMenuMode);
#endif  
          menu_current = 0; // Enter submenu and reset position to 1st menu item;
        } else {
          refreshMenuMode = MODE_MENU; // Refresh Main menu to update navigation;
        }
    } else
    if (menuMode == MODE_RADIO) { // If key is pressed in Radio menu, update it;
        updateMenuRadio(key);
        refreshMenuMode = MODE_RADIO; // Refresh Radio menu to update navigation;
    } else
    if (menuMode == MODE_CHANNELS 
#ifdef RC_MIN_MAX    
        || menuMode == MODE_CHANNELS_MIN_MAX
#endif    
    ) { // Exit from Channels menu by a key;
        beeper1.play(&SEQ_KEY_SELECT);
        refreshMenuMode = MODE_MENU; // Show Main menu;
    } else
    if (menuMode == MODE_SCREENSAVER) { // If key is pressed in Screensaver mode, select Main menu, reset menu line;
        beeper1.play(&SEQ_KEY_SELECT);
        refreshMenuMode = MODE_MENU; // Show Main menu;
#ifdef DEBUG
  Serial.print("menuLoop2=0"); Serial.println(refreshMenuMode);
#endif  
        menu_current = 0; // Enter Main menu and reset position to 1st menu item;
    }
  } else {
    if (menuMode != MODE_SCREENSAVER && 
        timer1.isTriggered(TIMER_SCREENSAVER)) { // If in some menu Screensaver timeout is triggered, return to Screensaver mode;
        refreshMenuMode = MODE_SCREENSAVER;
    }
  }
  last_key_code = key;

  // Check for additional menu refreshes if something is flashing or displayed data updated; 
  switch(menuMode) {
    case MODE_RADIO:
      // Beeps if needed in Range Check or Bind modes;
      handleBeeps();
      // In Radio menu refresh in flashing in Range Check or Bind modes need it; 
      if (update_menu_flash_timer() && (PXX.getModeRangeCheck() || PXX.getModeBind())) {
        refreshMenuMode = menuMode;
      }
      break;
    case MODE_SCREENSAVER:
      // Refresh Screensaver if CPPM lost/obtained;
      if (cppmModeChanged()) {
          refreshMenuMode = menuMode;
      }
      // Refresh if Relay mode switched;
#ifdef RELAY      
      if (isRelayActiveChanged()) {
          refreshMenuMode = menuMode;
      }
#endif      
      // Refresh Screensaver if battery voltage changed (with the given update period);
      if (timer1.isTriggered(TIMER_BATTERY_SCREEN)) {
        if (battery1.isVoltageChanged()) {
          refreshMenuMode = menuMode;
        }
      }
      break;
    case MODE_CHANNELS:
#ifdef RC_MIN_MAX    
    case MODE_CHANNELS_MIN_MAX:
#endif    
      // Refresh Channels with the given update period;
      if (timer1.isTriggered(TIMER_CHANNELS_SCREEN)) {
        refreshMenuMode = menuMode;
      }
      break;
  }

  // Redraw screens if resfresh required;
  switch(refreshMenuMode) {
    case MODE_RADIO: 
      showMenuRadio();
      break;
    case MODE_SCREENSAVER:
      showScreenSaver();
      break;
    case MODE_MENU:
      showMenu();
      break;
    case MODE_CHANNELS:
#ifdef RC_MIN_MAX    
    case MODE_CHANNELS_MIN_MAX:
#endif    
      showChannels(refreshMenuMode);
      break;
#ifdef RELAY      
    case MODE_RELAY:
      showRelay();
      break;
#endif      
  }
#endif // #ifdef OLED

#ifndef OLED
  if (cliActive) {
    sCmd.readSerial();
    if (timer1.isTriggered(TIMER_CLI_PERIODIC)) {
      handlePeriodic();
    }
  }
#endif // #ifndef OLED
}

#ifndef OLED
void setCliActive(bool value) {
  cliActive = value;
}

bool isCliActive() {
  return cliActive;
}
#endif

void setCPPM_Start(void) {
#ifdef DEBUG
  Serial.println("CPPM.Start");
#endif
    cppm_state = CPPM_START;
    timer1.suspend(TIMER_NO_CPPM);
}

void setCPPM_Obtained(void) {
    if (cppm_state == CPPM_LOST) {
#ifdef DEBUG
  Serial.println("CPPM.Lost->Got");
#endif
      beeper1.stop(&SEQ_MODE_NO_CPPM);
      beeper1.stop(&SEQ_MODE_CPPM_LOST);
      beeper1.play(&SEQ_MODE_GOT_CPPM);
      timer1.suspend(TIMER_NO_CPPM);
    }
#ifdef DEBUG
    if (cppm_state == CPPM_START) {
  Serial.println("CPPM.Start->Got");
    }
#endif
    cppm_state = CPPM_OBTAINED;  
}

void setCPPM_Lost(void) {
    if (cppm_state == CPPM_OBTAINED) {
#ifdef DEBUG
  Serial.println("CPPM.Got->Lost");
#endif
      beeper1.stop(&SEQ_MODE_GOT_CPPM);
      beeper1.play(&SEQ_MODE_CPPM_LOST);
      timer1.resume(TIMER_NO_CPPM);
      cppm_state = CPPM_LOST;  
    } else
    if (cppm_state == CPPM_LOST) {
      if (timer1.isTriggered(TIMER_NO_CPPM)) {
#ifdef DEBUG
  Serial.println("CPPM.Lost.Seq");
#endif
        beeper1.play(&SEQ_MODE_NO_CPPM);
      }
    }
}

bool cppmModeChanged(void) {
    if (old_cppm_state != cppm_state) {
      old_cppm_state = cppm_state;
      return true;
    }
    return false;
}

#ifndef OLED

bool readBooleanValue(bool& valueVar, bool doSave = true) {
  char *name = sCmd.next();
  if (strcasecmp(name,"on") == 0) {
    valueVar = true;
  } else
  if (strcasecmp(name,"off") == 0) {
    valueVar = false;
  } else {
    Serial.println(F("#- Boolean value should be 'on' or 'off': "));
    return false;
  }
  if (doSave)
    write_settings();
  return true;
}

void printBooleanValue(bool value) {
  Serial.print(value ? F("on") : F("off"));  
}

bool cmd_set_mode() {
  char *name = sCmd.next();
  if (strcasecmp(name,"US") == 0)
    radioMode = RADIO_US_FCC;
  else if (strcasecmp(name,"EU") == 0) 
    radioMode = RADIO_EU_LBT; 
  else if (strcasecmp(name,"FLEX") == 0) 
    radioMode = RADIO_EU_FLEX; 
  else {
    Serial.print(F("#- Invalid mode (US|EU|FLEX): "));
    Serial.print(name);
    Serial.println();
    return false;
  }
  adjust_settings();
  write_settings();
  return true;
}

void cmd_get_mode() {
  switch(radioMode) {
    case RADIO_US_FCC: Serial.print(F("US")); break;
    case RADIO_EU_LBT: Serial.print(F("EU")); break;
    case RADIO_EU_FLEX: Serial.print(F("FLEX")); break;
  }
}

void cmd_get_power() {
  if (radioMode != RADIO_EU_LBT) { // RADIO_US_FCC || RADIO_EU_FLEX
    switch (PXX.getPower()) {
  case 0: Serial.print(F("10")); break;
  case 1: Serial.print(F("100")); break;
  case 2: Serial.print(F("500")); break;
  case 3: Serial.print(F("1000")); break;
    }
  } else {
    switch (PXX.getPower()) {
  case 0: Serial.print(F("25_8ch")); break;
  case 1: Serial.print(F("25")); break;
  case 2: Serial.print(F("200")); break;
  case 3: Serial.print(F("500")); break;
    }    
  }
}

bool cmd_set_power() {
  char *name = sCmd.next();
  if (radioMode != RADIO_EU_LBT) { // RADIO_US_FCC || RADIO_EU_FLEX
    if (strcasecmp(name,"10") == 0)
      PXX.setPower(0);
    else if (strcasecmp(name,"100") == 0)
      PXX.setPower(1);
    else if (strcasecmp(name,"500") == 0)
      PXX.setPower(2);
    else if (strcasecmp(name,"1000") == 0)
      PXX.setPower(3);
    else {
      Serial.print(F("#- Invalid US/FLEX mode power (10|100|500|1000): "));
      Serial.print(name);
      Serial.println();
      return false;
    }
  } else {
    if (strcasecmp(name,"25_8ch") == 0)
      PXX.setPower(0);
    else if (strcasecmp(name,"25") == 0)
      PXX.setPower(1);
    else if (strcasecmp(name,"200") == 0)
      PXX.setPower(2);
    else if (strcasecmp(name,"500") == 0)
      PXX.setPower(3);
    else {
      Serial.print(F("#- Invalid EU mode power (25_8ch|25|200|500): "));
      Serial.print(name);
      Serial.println();
      return false;
    }
  }
  adjust_settings();
  write_settings();
  return true;
}

bool cmd_set_rx() {
  char *val = sCmd.next();
  if (!val) {
    Serial.println(F("#- Receiver number missing"));
    return false;
  }
  int value = atoi(val);
  if (value < 0 || value > 255) {
    Serial.println(F("#- Receiver number should be from 0 to 255"));
    return false;    
  }
  PXX.setRxNum(value);
  write_settings();
  return true;
}

void cmd_get_rx() {
  Serial.print(PXX.getRxNum());
}

bool cmd_set_telem() {
  bool value;
  if (!readBooleanValue(value, false))
    return false;
  PXX.setTelemetry(value);
  write_settings();
  return true;
}

void cmd_get_telem() {
  printBooleanValue(PXX.getTelemetry());
}

bool cmd_set_sport() {
  bool value;
  if (!readBooleanValue(value, false))
    return false;
  PXX.setSPort(value);
  write_settings();
  return true;
}

void cmd_get_sport() {
  printBooleanValue(PXX.getSPort());
}

#ifdef RELAY

bool readChannelNumber(uint8_t& channelVar) {
  char *name = sCmd.next();
  if (!name) {
    Serial.println(F("#- Channel name missing"));
    return false;
  }
  if (strncasecmp(name, "CH", 2)) { // != 0
    Serial.println(F("#- Channel name should start with 'CH'"));
    return false; 
  }
  int channel = atoi(name+2);
  if (channel < 5 || channel > 8) {
    Serial.println(F("#- Channel name should be CH5 to CH8"));
    return false;
  }
  channelVar = channel-1;
  write_settings();
  return true;
}

void printChannelNumber(uint8_t channel) {
  Serial.print(F("CH"));  
  Serial.print(channel+1, DEC);  
}

bool readChannelValue(uint16_t& valueVar) {
  char *name = sCmd.next();
  if (!name) {
    Serial.println(F("#- Channel value missing"));
    return false;
  }
  int value = atoi(name);
  if (value < 900 || value > 2100) {
    Serial.println(F("#- Channel value should be in range [900..2100]"));
    return false;
  }
  valueVar = value;
  write_settings();
  return true;
}

void printChannelValue(uint16_t value) {
  Serial.print(value, DEC);
}

bool cmd_set_relayEnabled()   { return readBooleanValue(relayEnabled); }
void cmd_get_relayEnabled()   { printBooleanValue(relayEnabled); }
bool cmd_set_relayChannel()   { return readChannelNumber(relayChannel); }
void cmd_get_relayChannel()   { printChannelNumber(relayChannel); }
bool cmd_set_gpsModeChannel() { return readChannelNumber(gpsModeChannel); }
void cmd_get_gpsModeChannel() { printChannelNumber(gpsModeChannel); }
bool cmd_set_gpsHoldValue()   { return readChannelValue(gpsHoldValue); }
void cmd_get_gpsHoldValue()   { printChannelValue(gpsHoldValue); }
bool cmd_set_activePXX_Min()  { return readChannelValue(activePXX_Min); }
void cmd_get_activePXX_Min()  { printChannelValue(activePXX_Min); }
bool cmd_set_activePXX_Max()  { return readChannelValue(activePXX_Max); }
void cmd_get_activePXX_Max()  { printChannelValue(activePXX_Max); }
bool cmd_set_activeCPPM_Min() { return readChannelValue(activeCPPM_Min); }
void cmd_get_activeCPPM_Min() { printChannelValue(activeCPPM_Min); }
bool cmd_set_activeCPPM_Max() { return readChannelValue(activeCPPM_Max); }
void cmd_get_activeCPPM_Max() { printChannelValue(activeCPPM_Max); }

bool cmd_set_holdThrottleEnabled() { return readBooleanValue(holdThrottleEnabled); }
void cmd_get_holdThrottleEnabled() { printBooleanValue(holdThrottleEnabled); }
bool cmd_set_midThrottle()    { return readChannelValue(midThrottle); }
void cmd_get_midThrottle()    { printChannelValue(midThrottle); }
bool cmd_set_minThrottle()    { return readChannelValue(minThrottle); }
void cmd_get_minThrottle()    { printChannelValue(minThrottle); }
bool cmd_set_safeThrottle()   { return readChannelValue(safeThrottle); }
void cmd_get_safeThrottle()   { printChannelValue(safeThrottle); }
bool cmd_set_armCPPMChannel() { return readChannelNumber(armCPPMChannel); }
void cmd_get_armCPPMChannel() { printChannelNumber(armCPPMChannel); }
bool cmd_set_armPXXChannel()  { return readChannelNumber(armPXXChannel); }
void cmd_get_armPXXChannel()  { printChannelNumber(armPXXChannel); }
bool cmd_set_armCPPM_Min()    { return readChannelValue(armCPPM_Min); }
void cmd_get_armCPPM_Min()    { printChannelValue(armCPPM_Min); }
bool cmd_set_armCPPM_Max()    { return readChannelValue(armCPPM_Max); }
void cmd_get_armCPPM_Max()    { printChannelValue(armCPPM_Max); }
bool cmd_set_armPXX_Min()     { return readChannelValue(armPXX_Min); }
void cmd_get_armPXX_Min()     { printChannelValue(armPXX_Min); }
bool cmd_set_armPXX_Max()     { return readChannelValue(armPXX_Max); }
void cmd_get_armPXX_Max()     { printChannelValue(armPXX_Max); }

#endif

#ifdef SHOW_RSSI

bool readADCValue(int& valueVar) {
  char *name = sCmd.next();
  if (!name) {
    Serial.println(F("#- ADC value missing"));
    return false;
  }
  int value = atoi(name);
  if (value < 0 || value > 1023) {
    Serial.println(F("#- ADC value should be in range [0..1023]"));
    return false;
  }
  valueVar = value;
  write_settings();
  return true;
}

void printADCValue(uint16_t value) {
  Serial.print(value, DEC);
}

bool cmd_set_vrx_rssi_min()  { return readADCValue(vrx_rssi_min); }
void cmd_get_vrx_rssi_min()  { printADCValue(vrx_rssi_min); }
bool cmd_set_vrx_rssi_max()  { return readADCValue(vrx_rssi_max); }
void cmd_get_vrx_rssi_max()  { printADCValue(vrx_rssi_max); }

#endif

struct Variable {
  const char *name;
  bool(*setter)();
  void(*getter)();
  const __FlashStringHelper * help;
};

static const char PROGMEM HELP_mode[] = "R9 frequency mode [US|EU|FLEX]";
static const char PROGMEM HELP_power[] = "R9 power US/FLEX: [10|100|500|1000], EU: [25_8ch|25|200|500]";
static const char PROGMEM HELP_rx[] = "R9 receiver number (0=any) [0..255]";
static const char PROGMEM HELP_telem[] = "R9 telemetry enabled or disabled [on|off]";
static const char PROGMEM HELP_sport[] = "R9 S.Port enabled or disabled [on|off]";
static const char PROGMEM HELP_relayEnabled[] = "Two-copters relay mode enabled or disabled [on|off]";
static const char PROGMEM HELP_relayChannel[] = "Channel to switch between PXX and PPM control [CH5..CH8]";
static const char PROGMEM HELP_gpsModeChannel[] = "Channel to enable GPS HOLD mode [CH5..CH8]";
static const char PROGMEM HELP_gpsHoldValue[] = "Value to enable GPS HOLD mode in gps_mode_channel (same on relay(CPPM) and mission(PXX) drone) [900..2100]";
static const char PROGMEM HELP_activePXX_Min[] = "Min. value in relay_channel to make mission(PXX) drone active [900..2100]";
static const char PROGMEM HELP_activePXX_Max[] = "Max. value in relay_channel to make mission(PXX) drone active [900..2100]";
static const char PROGMEM HELP_activeCPPM_Min[] = "Min. value in relay_channel to make relay(CPPM) drone active [900..2100]";
static const char PROGMEM HELP_activeCPPM_Max[] = "Max. value in relay_channel to make relay(CPPM) drone active [900..2100]";

#ifdef SHOW_RSSI
static const char PROGMEM HELP_vrx_rssi_min[] = "Min. ADC value for for Video RX Rssi (0%) [0..1023]";
static const char PROGMEM HELP_vrx_rssi_max[] = "Max. ADC value for for Video RX Rssi (100%) [0..1023]";
#endif

static const char PROGMEM HELP_holdThrottleEnabled[] = "Setting mid-throttle for inactive drone, enabled or disabled [on|off]";
static const char PROGMEM HELP_midThrottle[] = "Channel value for middle of throttle (normally 1500) [900..2100]";
static const char PROGMEM HELP_minThrottle[] = "Channel value for minimum throttle when motors are off (normally 1000) [900..2100]";
static const char PROGMEM HELP_safeThrottle[] = "Channel value for minimum throttle when copter considered flying (i.e. 1100) [900..2100]";
static const char PROGMEM HELP_armPXXChannel[] = "Channel to arm PXX drone [CH5..CH8]";
static const char PROGMEM HELP_armCPPMChannel[] = "Channel to arm CPPM drone [CH5..CH8]";
static const char PROGMEM HELP_armPXX_Min[] = "Min. value in arm channel when PXX drone is armed [900..2100]";
static const char PROGMEM HELP_armPXX_Max[] = "Max. value in arm channel when PXX drone is armed [900..2100]";
static const char PROGMEM HELP_armCPPM_Min[] = "Min. value in arm channel when CPPM drone is armed [900..2100]";
static const char PROGMEM HELP_armCPPM_Max[] = "Max. value in arm channel when CPPM drone is armed [900..2100]";

static Variable CONFIG_VARS[] = {
   {"R9_mode",          cmd_set_mode,           cmd_get_mode,           FPSTR(HELP_mode)}
  ,{"R9_power",         cmd_set_power,          cmd_get_power,          FPSTR(HELP_power)}
  ,{"R9_rx",            cmd_set_rx,             cmd_get_rx,             FPSTR(HELP_rx)}
  ,{"R9_telem",         cmd_set_telem,          cmd_get_telem,          FPSTR(HELP_telem)}
  ,{"R9_sport",         cmd_set_sport,          cmd_get_sport,          FPSTR(HELP_sport)}
#ifdef SHOW_RSSI
  ,{"vrx_rssi_min",     cmd_set_vrx_rssi_min,   cmd_get_vrx_rssi_min,   FPSTR(HELP_vrx_rssi_min)}
  ,{"vrx_rssi_max",     cmd_set_vrx_rssi_max,   cmd_get_vrx_rssi_max,   FPSTR(HELP_vrx_rssi_max)}
#endif
#ifdef RELAY
  ,{"relay_enabled",    cmd_set_relayEnabled,   cmd_get_relayEnabled,   FPSTR(HELP_relayEnabled)}
  ,{"relay_channel",    cmd_set_relayChannel,   cmd_get_relayChannel,   FPSTR(HELP_relayChannel)}
  ,{"gps_mode_channel", cmd_set_gpsModeChannel, cmd_get_gpsModeChannel, FPSTR(HELP_gpsModeChannel)}
  ,{"gps_hold_value",   cmd_set_gpsHoldValue,   cmd_get_gpsHoldValue,   FPSTR(HELP_gpsHoldValue)}
  ,{"active_PXX_min",   cmd_set_activePXX_Min,  cmd_get_activePXX_Min,  FPSTR(HELP_activePXX_Min)}
  ,{"active_PXX_max",   cmd_set_activePXX_Max,  cmd_get_activePXX_Max,  FPSTR(HELP_activePXX_Max)}
  ,{"active_CPPM_min",  cmd_set_activeCPPM_Min, cmd_get_activeCPPM_Min, FPSTR(HELP_activeCPPM_Min)}
  ,{"active_CPPM_max",  cmd_set_activeCPPM_Max, cmd_get_activeCPPM_Max, FPSTR(HELP_activeCPPM_Max)}

  ,{"hold_throttle_enabled",  cmd_set_holdThrottleEnabled, cmd_get_holdThrottleEnabled, FPSTR(HELP_holdThrottleEnabled)}
  ,{"min_throttle",     cmd_set_minThrottle,    cmd_get_minThrottle,    FPSTR(HELP_minThrottle)}
  ,{"mid_throttle",     cmd_set_midThrottle,    cmd_get_midThrottle,    FPSTR(HELP_midThrottle)}
  ,{"safe_throttle",    cmd_set_safeThrottle,   cmd_get_safeThrottle,   FPSTR(HELP_safeThrottle)}
  ,{"arm_PXX_channel",  cmd_set_armPXXChannel,  cmd_get_armPXXChannel,  FPSTR(HELP_armPXXChannel)}
  ,{"arm_PXX_min",      cmd_set_armPXX_Min,     cmd_get_armPXX_Min,     FPSTR(HELP_armPXX_Min)}
  ,{"arm_PXX_max",      cmd_set_armPXX_Max,     cmd_get_armPXX_Max,     FPSTR(HELP_armPXX_Max)}
  ,{"arm_CPPM_channel", cmd_set_armCPPMChannel, cmd_get_armCPPMChannel, FPSTR(HELP_armCPPMChannel)}
  ,{"arm_CPPM_min",     cmd_set_armCPPM_Min,    cmd_get_armCPPM_Min,    FPSTR(HELP_armCPPM_Min)}
  ,{"arm_CPPM_max",     cmd_set_armCPPM_Max,    cmd_get_armCPPM_Max,    FPSTR(HELP_armCPPM_Max)}
#endif
};

#define CONFIG_VARS_NUM (sizeof(CONFIG_VARS) / sizeof(Variable))

void cmd_set_get(bool set) {
  char *name = sCmd.next();
  if (!name) {
    Serial.println(F("#- Variable name expected"));
    return;
  }
  for (int i=0; i<CONFIG_VARS_NUM; i++) {
    Variable* var = &CONFIG_VARS[i]; 
    if (strcasecmp(name, var->name) == 0) {
      if (!set) { // get
        Serial.print("# ");
        Serial.print(name);
        Serial.print(" is ");
        var->getter();
        Serial.println();
      } else {
        bool success = var->setter();
        if (success) {
          Serial.print(F("# "));
          Serial.print(name);
          Serial.print(" set to ");
          var->getter();
          Serial.println();
        }
      }
      return;
    }
  }
  Serial.print(F("#- Unknown variable: "));
  Serial.println(name);
}

void cmd_set() {
  cmd_set_get(true);
}

void cmd_get() {
  cmd_set_get(false);
}

void cmd_dump() {
  Serial.println(F("# Dump:"));
  for (int i=0; i<CONFIG_VARS_NUM; i++) {
    Variable* v = &CONFIG_VARS[i];
    Serial.print(F("set "));
    Serial.print(v->name);
    Serial.print(" ");
    v->getter();
    if (v->help) {
      Serial.print(F(" ; "));
      Serial.print(v->help);
    }
    Serial.println();
  }
}

static void(*periodic_func)(bool) = NULL;

void handlePeriodic() {
  if (periodic_func)
    periodic_func(true);
}

void cmd_periodic(void(*f)(bool), bool isPeriodicOnly) {
  char *arg = sCmd.next();
  if (!arg || !strlen(arg)) {
    if (isPeriodicOnly) {
      Serial.println(F("#- This command requires on/off parameter."));
    } else {
      f(true);
    }
  } else
  if (strcasecmp(arg, "on") == 0) {
    if (periodic_func) {
      Serial.println(periodic_func == f ? F("#- This process is already running.") : F("#- Other process is already running."));
      return;
    }
    f(true);
    periodic_func = f;
  } else
  if (strcasecmp(arg, "off") == 0) {
    if (!periodic_func) {
      Serial.println(F("#- No process is running."));
      return;
    }
    if (periodic_func != f) {
      Serial.println(F("#- Other process is running."));
      return;
    }
    f(false);
    periodic_func = NULL;
  } else {
    Serial.print(F("#- Not on/off parameter: "));
    Serial.println(arg);
  }
}

void handle_status(bool active) {
  if (!active)
    return;
  Serial.print(F("# Status (ver. "));
  Serial.print(VERSION_NUMBER);
  Serial.println(F(")"));

  Serial.print(F("Build options: "));
#ifdef OLED
  Serial.print(F("OLED "));
#endif  
#ifdef SOUND_OFF
  Serial.print(F("SOUND_OFF "));
#endif  
#ifdef BEEP_FAILSAFE
  Serial.print(F("BEEP_FAILSAFE "));
#endif  
#ifdef RC_MIN_MAX
  Serial.print(F("RC_MIN_MAX "));
#endif  
#ifdef EMIT_CPPM
  Serial.print(F("EMIT_CPPM "));
#endif  
#ifdef RELAY
  Serial.print(F("RELAY "));
#endif  
#ifdef SHOW_RSSI
  Serial.print(F("SHOW_RSSI "));
#endif
  Serial.println();

  char buf[5];
  Serial.print(F("Battery "));
  Serial.print(battery1.getNumCells(), DEC);
  Serial.print(F("s: "));
  dtostrf(battery1.getCurrVoltage(), 4, 1, buf);
  Serial.print(buf);
  Serial.println("v");

  const __FlashStringHelper* s;
  switch (cppm_state) {
  case CPPM_START: s = F("RX: Wait");
       break;
  case CPPM_OBTAINED: s = F("RX: OK");
       break;
  default: s = F("RX: Lost"); // CPPM_LOST
       break;
  }
  Serial.print(s);

  if (CPPM.getFailReason()) {
    Serial.print(F(", Fail reason: "));
      Serial.println(CPPM.getFailReason(), DEC);
  } else {
    Serial.println();
  }

  Serial.print(F("Band: "));
  Serial.print(getBandStrShort());
  Serial.print(F(", "));
  Serial.println(getPowerStr()); 

#ifdef RELAY
  Serial.print(F("Relay: "));
  if (relayEnabled) {
    switch (getRelayActive()) {
      case RELAY_ACTIVE_PXX: s = F("PXX");  break;
      case RELAY_ACTIVE_CPPM: s = F("CPPM"); break; 
      default: s = F("---");
    }
    Serial.println(s);

    Serial.print(F("Hold throttle control is "));    
    if (holdThrottleEnabled) {
      Serial.print(F("ON. CPPM: "));
      Serial.print(getCPPMArmed() ? F("ARMED, ") : F("disarmed, "));
      Serial.print(getCPPMFlying() ? F("FLYING") : F("landed"));
      Serial.print(F(", PXX: "));
      Serial.print(getPXXArmed() ? F("ARMED, ") : F("disarmed, "));
      Serial.println(getPXXFlying() ? F("FLYING") : F("landed"));
    } else {
      Serial.println(F("OFF"));    
    }
  } else {
    Serial.println(F("OFF"));    
  }
#endif

  if (PXX.getModeRangeCheck()) Serial.println(F("RANGE CHECK in progress"));
  if (PXX.getModeBind()) Serial.println(F("BIND in progress"));
}

void cmd_status() { cmd_periodic(handle_status, false); }

void handle_channels(bool active) {
  if (!active)
    return;
  Serial.println(F("# Channels:"));

  for (int8_t i=0; i<NUM_CHANNELS_CPPM; i++) {
    Serial.print(F("CH"));
    Serial.print(i+1, DEC);
    if (cppmActive) {
      int16_t x = channels[i];
      Serial.print(channelValid(x) ? F(":  ") : F(": *"));
      Serial.print(x, DEC);
    } else {
      Serial.print(F(": ----"));
    }
#ifdef RC_MIN_MAX
    if (channelsMinMaxSet) {
      Serial.print(F(" ["));
      Serial.print(channelsMin[i], DEC);
      Serial.print(F(".."));
      Serial.print(channelsMax[i], DEC);
      Serial.print(']');
    }
#endif
    Serial.println();
  }
  Serial.print(F("Errors: "));
  Serial.print(CPPM.getErrorCount(), DEC);
  Serial.print(F(" Fail reason: "));
  Serial.println(CPPM.getFailReason(), DEC);
} 

void cmd_channels() { cmd_periodic(handle_channels, false); }

void handle_bind(bool active) {
  PXX.setModeBind(active);
  Serial.println(active ? F("# Binding...") : F("# Bind finished."));
} 

void cmd_bind() { cmd_periodic(handle_bind, true); }

void handle_rangecheck(bool active) {
  PXX.setModeRangeCheck(active);
  Serial.println(active ? F("# Range checking...") : F("# Range check finished."));
} 

void cmd_rangecheck() { cmd_periodic(handle_rangecheck, true); }

#ifdef SHOW_RSSI
void handle_vrx_rssi(bool active) {
  if (!active)
    return;
    
  Serial.print(F("# VRx RSSI: "));
  Serial.print(getVRx_Rssi(), DEC);
  Serial.print(F("% ["));
  Serial.print(getVRx_Rssi_raw(), DEC);
  Serial.println("]");
} 

void cmd_vrx_rssi() { cmd_periodic(handle_vrx_rssi, false); }
#endif

void cmd_reset() {  
  init_settings();
  write_settings();
  Serial.println(F("# Settings were reset"));
}

void cmd_help() {
  Serial.println(F("# Help"));
  Serial.println(F("set <variable> <value> ; set variable value"));
  Serial.println(F("get <variable>         ; get variable value"));
  Serial.println(F("dump                   ; print all variable values"));
  Serial.println(F("status [on|off]        ; show status [periodically]"));
  Serial.println(F("channels [on|off]      ; show channel values [periodically]"));
  Serial.println(F("bind on|off            ; turn BIND mode on|off"));
  Serial.println(F("rangecheck on|off      ; turn RangeCheck on|off"));
#ifdef SHOW_RSSI
  Serial.println(F("vrx_rssi on|off        ; show VRx RSSI [periodically]"));
#endif  
  Serial.println(F("reset                  ; revert to factory settings"));
  Serial.println(F("help                   ; show the help"));
}

void unrecognized(const char *command) {
  Serial.print(F("#- Unknown Command: "));
  Serial.println(command);
}

void init_commands(void) {
  sCmd.addCommand("set", cmd_set); // set <value> <value>
  sCmd.addCommand("get", cmd_get); // get <variable>
  sCmd.addCommand("dump", cmd_dump);
  sCmd.addCommand("status", cmd_status); // status | status on | status off
  sCmd.addCommand("channels", cmd_channels); // channels | channels on | channels off
  sCmd.addCommand("bind", cmd_bind); // bind on | bind off
  sCmd.addCommand("rangecheck", cmd_rangecheck); // rangecheck on | rangecheck off
#ifdef SHOW_RSSI
  sCmd.addCommand("vrx_rssi", cmd_vrx_rssi); // vrx_rssi | vrx_rssi on | vrx_rssi off
#endif  
  sCmd.addCommand("reset", cmd_reset);
  sCmd.addCommand("help", cmd_help);
  sCmd.setDefaultHandler(unrecognized);	// Handler for command that isn't matched  (says "Unknown")
  sCmd.clearBuffer();
}

#endif // #ifndef OLED
