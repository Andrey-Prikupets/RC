#include <Arduino.h>
#include <avr/pgmspace.h>

#include <U8g2lib.h>

#include <EEPROM.h>

#include <SoftwareSerial.h>

SoftwareSerial HC12(10, 11); // HC-12 TX Pin, HC-12 RX Pin

#define PIN_SET 12

#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))

//#include "debug.h"

/*
#ifdef DEBUG // Allocate less display memory in DEBUG mode;
  U8G2_SSD1306_64X48_ER_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#else
  U8G2_SSD1306_64X48_ER_F_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#endif
*/

uint8_t char_height;
uint8_t char_width;
uint8_t screen_width;
uint8_t screen_height;

// Display selection; Note: after changing display may need to re-design menu;
#ifdef DEBUG // Allocate less display memory in DEBUG mode;
  U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
  // U8G2_SSD1306_64X48_ER_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#else
  // U8G2_SSD1306_64X48_ER_F_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
  U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;
#endif


// SeralCommand -> https://github.com/kroimon/Arduino-SerialCommand.git
#include <SerialCommand.h>

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
  u8g.setCursor((uint8_t) screen_width-x-getLength_F(s_P)*char_width, y);
  u8g.print(s_P);
}

void drawCentered_F(int y, const __FlashStringHelper* s_P) {
  u8g.setCursor((screen_width-(getLength_F(s_P)+1)*char_width)/2, y);     
  u8g.print(s_P);
}

void drawVal_F(uint8_t x, uint8_t y, unsigned int value, const __FlashStringHelper* s_P) {
  char buf[10];
  u8g.setCursor(x, y);
  u8g.print(s_P);
  itoa(value, buf, 10);
  u8g.print(buf);  
}

void prepare(void) {
  u8g.setFont(u8g_font_5x8); // u8g_font_6x10
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDrawColor(1);

  char_height = u8g.getMaxCharHeight(); // u8g.getFontAscent()-u8g.getFontDescent()+1;    
  char_width = u8g.getStrWidth("W")+1;
  screen_width = (uint8_t) u8g.getWidth();
  screen_height = (uint8_t) u8g.getHeight();
  
  Serial.println(); 
  Serial.print(F("char ")); Serial.print(char_width); Serial.print("x"); Serial.print(char_height); 
  Serial.print(F(" screen ")); Serial.print(screen_width); Serial.print("x"); Serial.println(screen_height); 
}

#define MAX_PACKET_LEN 25
#define MAX_LINES 14

static char lines[MAX_LINES][MAX_PACKET_LEN+1];
static uint8_t buf_pos = 0;
static uint16_t packet = 0;

char* getLine(uint8_t back_ofst = 0) {
  return lines[(packet-back_ofst) % MAX_LINES];
}

void sendCommand(char* command) {
  digitalWrite(PIN_SET, LOW);
  delay(100);
  HC12.write(command);
  HC12.flush();
  delay(500);
  digitalWrite(PIN_SET, HIGH);
  delay(100);
}

bool echo = false;

void handlePacket(void) {
    char* buf = getLine();
    Serial.println(); Serial.print("Packet: "); Serial.print(packet); Serial.print(" Len="); Serial.println(strlen(buf));
    drawPackets();
    if (echo) {
      HC12.write(buf);
      HC12.println();
      HC12.flush();
    }
    if (strncmp(buf, "AT+ECHO=0", 9) == 0) {
      echo = false;
      HC12.println("ECHO=0");
      HC12.flush();
    } else  
    if (strncmp(buf, "AT+ECHO=1", 9) == 0) {
      echo = true;
      HC12.println("ECHO=1");
      HC12.flush();
    } else  
    if (strncmp(buf, "AT+", 3) == 0) {
      sendCommand(buf);
      Serial.print("Command: "); Serial.println(packet);
    }  
}

void drawPackets() {
  // picture loop  
  
  u8g.firstPage();
  do  {
    drawVal_F(0, 0, packet+1, F("Pkts: "));
    drawVal_F(screen_width/2, 0, buf_pos, F("Len: "));
    int x = screen_width/2-1;
    u8g.drawLine(x, char_height, x, screen_height-1);
    u8g.drawLine(0, char_height, screen_width, char_height);
    
    int8_t start_packet_ofs = packet < MAX_LINES ? packet : MAX_LINES-1;
    int8_t col2_start = MAX_LINES/2;
    x = 0;
    uint8_t y_ofs = 0;
    uint8_t n = 0;
    for (int8_t i=start_packet_ofs; i>=0; i--) {
      char* buf = getLine(i);
      if (n == col2_start) {
        x = (screen_width/2)+2;
        y_ofs = 0;
      }
      u8g.setCursor(x, char_height*(y_ofs+1)+1);
      u8g.print(buf);
      y_ofs++;
      n++;
    }
  } while( u8g.nextPage() );
}

void drawLogo(void) {
  // picture loop  
  u8g.firstPage();
  do  {
    drawCentered_F(0, F("iNAV Radar for HC-12"));
    drawCentered_F(char_height*2, F("(c) by Andrey Prikupets"));
  } while( u8g.nextPage() );
  
  delay(1200);
}

void setup(void) {
  u8g.begin();
  Serial.begin(115200);             // Serial port to computer

  pinMode(PIN_SET, OUTPUT);
  digitalWrite(PIN_SET, HIGH); // 1=data mode; 0=AT commands mode;

  HC12.begin(2400);               // Serial port to HC12

  prepare();
  drawLogo();

  for (uint8_t i=0; i<MAX_LINES; i++) {
    lines[i][0] = 0;
  }
}

void loop(void) {
  bool completed = false;
  while (HC12.available() && buf_pos < MAX_PACKET_LEN && !completed) {
    char ch = HC12.read();
    Serial.write(ch);
    char* buf = getLine();
    if (buf_pos > 0 && ch == 0x0A && buf[buf_pos-1] == 0x0D) {
      completed = true;
      buf_pos--; // remove 0x0D;
      buf[buf_pos] = 0;
    } else {
      buf[buf_pos++] = ch;
    }
  }
  if (buf_pos >= MAX_PACKET_LEN || completed) {
    handlePacket();
    buf_pos = 0;
    packet++;
  }
}
