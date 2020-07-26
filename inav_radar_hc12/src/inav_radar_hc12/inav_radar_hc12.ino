#include <Arduino.h>
#include <avr/pgmspace.h>

#include <U8g2lib.h>

#include <EEPROM.h>

#include <SoftwareSerial.h>

#include "MSP.h"
#include "inav_radar_hc12.h"
#include <math.h>
///#include <cmath.h>

#include <ButtonDebounce.h>

#include <SerialTransfer.h>

SoftwareSerial HC12(PIN_HC12_RX, PIN_HC12_TX); // HC-12 TX Pin, HC-12 RX Pin

SerialTransfer hc12;

#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))

// Display selection; Note: after changing display may need to re-design menu;
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g(U8G2_R0); // hardware // F - fastest, 1 - slowest, 2 - medium;

// -------- VARS

uint8_t char_height;
uint8_t char_width;
uint8_t screen_width;
uint8_t screen_height;

config_t cfg;
system_t sys;
stats_t stats;
MSP msp;

msp_radar_pos_t radarPos;

curr_t curr; // Our peer ID
peer_t peers[LORA_NODES_MAX]; // Other peers

air_type0_t air_0;
air_type1_t air_1;
//air_type2_t air_2;
air_type1_t * air_r1;
//air_type2_t * air_r2;

ButtonDebounce button(PIN_BUTTON, 250); // debounce time

// -------- DISPLAY

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

void drawStr(uint8_t x, uint8_t y, const char* s) {
  u8g.setCursor(x, y);
  u8g.print(s);
}

inline void drawStr(const char* s) {
  u8g.print(s);
}

inline void drawStr(const __FlashStringHelper* s_P) {
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

void drawCentered(int y, char* s) {
  u8g.setCursor((screen_width-(strlen(s)+1)*char_width)/2, y);     
  u8g.print(s);
}

void drawVal(uint8_t x, uint8_t y, unsigned int value) {
  char buf[10];
  u8g.setCursor(x, y);
  itoa(value, buf, 10);
  u8g.print(buf);  
}

void drawVal(unsigned int value) {
  char buf[10];
  itoa(value, buf, 10);
  u8g.print(buf);  
}

void drawVal_F(uint8_t x, uint8_t y, unsigned int value, const __FlashStringHelper* s_P) {
  char buf[10];
  u8g.setCursor(x, y);
  itoa(value, buf, 10);
  u8g.print(buf);  
  u8g.print(s_P);
}

void drawVal_F(uint8_t x, uint8_t y, unsigned int value) {
  char buf[10];
  u8g.setCursor(x, y);
  itoa(value, buf, 10);
  u8g.print(buf);  
}

void drawVal_F(unsigned int value, const __FlashStringHelper* s_P) {
  char buf[10];
  itoa(value, buf, 10);
  u8g.print(buf);  
  u8g.print(s_P);
}

void drawVal_F(uint8_t x, uint8_t y, const __FlashStringHelper* s_P, char* s) {
  u8g.setCursor(x, y);
  u8g.print(s_P);
  u8g.print(s);  
}

void drawVal_F(uint8_t x, uint8_t y, const __FlashStringHelper* s_P, unsigned int value) {
  char buf[10];
  u8g.setCursor(x, y);
  u8g.print(s_P);
  itoa(value, buf, 10);
  u8g.print(buf);  
}

void drawVal_F(uint8_t x, uint8_t y, const __FlashStringHelper* s_P, unsigned int value, const __FlashStringHelper* s_P1) {
  char buf[10];
  u8g.setCursor(x, y);
  u8g.print(s_P);
  itoa(value, buf, 10);
  u8g.print(buf);  
  u8g.print(s_P1);
}

void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t progress) {
  u8g.drawFrame(x,y,w,h);  
  u8g.drawBox(x+1,y+1,(int) ((w-2)*progress+50)/100, h-2);  
}

// -------- HC12

void sendCommand(char* command) {
  digitalWrite(PIN_HC12_SET, LOW);
  delay(100);
  HC12.write(command);
  HC12.flush();
  delay(500);
  digitalWrite(PIN_HC12_SET, HIGH);
  delay(100);
}

// -------- SYSTEM

void set_mode(void) {
        cfg.channel = HC12_CHANNEL;
        cfg.power = HC12_POWER;
        cfg.lora_slot_spacing = 125;
        cfg.lora_nodes_max = LORA_NODES_MAX;
        cfg.lora_cycle = cfg.lora_nodes_max * cfg.lora_slot_spacing;
        cfg.lora_timing_delay = -60;
        cfg.lora_antidrift_threshold = 5;
        cfg.lora_antidrift_correction = 5;
        cfg.lora_peer_timeout = 6000;

//        cfg.lora_air_mode = LORA_NODES_MIN;

        cfg.msp_version = 2;
        cfg.msp_timeout = 100;
        cfg.msp_fc_timeout = 6000;
        cfg.msp_after_tx_delay = 85;

        cfg.cycle_scan = 4000;
        cfg.cycle_display = 250;
        cfg.cycle_stats = 1000;
}

// active == 1 => count found peers with not lost connection;
// action == 0 => count found peers regardless of connection;
int count_peers(bool active = 0) {
    int j = 0;
    for (int i = 0; i < LORA_NODES_MAX; i++) {
        if (active == 1) {
            if ((peers[i].id > 0) && !peers[i].lost) {
                j++;
            }
        }
        else {
            if (peers[i].id > 0) {
                j++;
            }
        }
    }
    return j;
}

void reset_peers() {
    sys.now_sec = millis();
    for (int i = 0; i < LORA_NODES_MAX; i++) {
        peers[i].id = 0;
        peers[i].host = 0;
        // peers[i].state = 0;
        peers[i].lost = 0;
        peers[i].broadcast = 0;
        peers[i].lq_updated = sys.now_sec;
        peers[i].lq_tick = 0;
        peers[i].lq = 0;
        peers[i].updated = 0;
        peers[i].distance = 0;
        peers[i].direction = 0;
        peers[i].relalt = 0;
        strcpy(peers[i].name, "");
    }
}

// Select first free peer and assign its number+1 as curr.id;
void pick_id() {
    curr.id = 0;
    for (int i = 0; i < LORA_NODES_MAX; i++) {
        if ((peers[i].id == 0) && (curr.id == 0)) {
            curr.id = i + 1;
        }
    }
}

// assign slot of next transmission;
void resync_tx_slot(int16_t delay) {
    bool startnow = 0;
    for (int i = 0; (i < LORA_NODES_MAX) && (startnow == 0); i++) { // Resync
        if (peers[i].id > 0) {
            sys.lora_next_tx = peers[i].updated + (curr.id - peers[i].id) * cfg.lora_slot_spacing + cfg.lora_cycle + delay;
            startnow = 1;
        }
    }
}

// ----------------------------------------------------------------------------- calc gps distance

double deg2rad(double deg) {
  return (deg * M_PI / 180);
}

double rad2deg(double rad) {
  return (rad * 180 / M_PI);
}

/**
 * Returns the distance between two points on the Earth.
 * Direct translation from http://en.wikipedia.org/wiki/Haversine_formula
 * @param lat1d Latitude of the first point in degrees
 * @param lon1d Longitude of the first point in degrees
 * @param lat2d Latitude of the second point in degrees
 * @param lon2d Longitude of the second point in degrees
 * @return The distance between the two points in meters
 */

double gpsDistanceBetween(double lat1d, double lon1d, double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = deg2rad(lat1d);
  lon1r = deg2rad(lon1d);
  lat2r = deg2rad(lat2d);
  lon2r = deg2rad(lon2d);
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return 2.0 * 6371000 * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

/*
double gpsDistanceBetween(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795;
}

*/

double gpsCourseTo(double lat1, double long1, double lat2, double long2)
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += TWO_PI;
  }
  return degrees(a2);
}

// -------- LoRa

void lora_send() {

    if (sys.lora_tick % 8 == 0) {
/*
        if (sys.lora_tick % 16 == 0) { 
            air_2.id = curr.id;
            air_2.type = 2;
            air_2.vbat = curr.fcanalog.vbat; // 1 to 255 (V x 10)
            air_2.mah = curr.fcanalog.mAhDrawn;
            air_2.rssi = curr.fcanalog.rssi; // 0 to 1023

            hc12.txObj(air_2);
        } else 
*/        
        {
            air_1.id = curr.id;
            air_1.type = 1;
            air_1.host = curr.host;
            // air_1.state = curr.state;
            air_1.broadcast = 0;
            air_1.speed = curr.gps.groundSpeed / 100; // From cm/s to m/s
            strncpy(air_1.name, curr.name, LORA_NAME_LENGTH);

            hc12.txObj(air_1);
        }
    }
    else {
        air_0.id = curr.id;
        air_0.type = 0;
        air_0.lat = curr.gps.lat / 100; // From XX.1234567 to XX.12345
        air_0.lon = curr.gps.lon / 100; // From XX.1234567 to XX.12345
        air_0.alt = curr.gps.alt; // m
        air_0.heading = curr.gps.groundCourse / 10;  // From degres x 10 to degres

        hc12.txObj(air_0);
    }
}

void lora_receive() {
    sys.lora_last_rx = millis();
    sys.lora_last_rx -= (stats.last_tx_duration > 0 ) ? stats.last_tx_duration : 0; // RX time should be the same as TX time

    sys.ppsc++;

    hc12.rxObj(air_0);

    uint8_t id = air_0.id - 1;
    sys.air_last_received_id = air_0.id;
    peers[id].id = sys.air_last_received_id;
    peers[id].lq_tick++;
    peers[id].lost = 0;
    peers[id].updated = sys.lora_last_rx;

    if (air_0.type == 1) { // Type 1 packet (Speed + host + state + broadcast + name)

        air_r1 = (air_type1_t*)&air_0;

        peers[id].host = (*air_r1).host;
        // peers[id].state = (*air_r1).state;
        peers[id].broadcast = (*air_r1).broadcast;
        peers[id].gps.groundSpeed = (*air_r1).speed * 100; // From m/s to cm/s
        strncpy(peers[id].name, (*air_r1).name, LORA_NAME_LENGTH);
        peers[id].name[LORA_NAME_LENGTH] = 0;

    }
/*    
    else if (air_0.type == 2) { // Type 2 packet (vbat mAh RSSI)

        air_r2 = (air_type2_t*)&air_0;

        peers[id].fcanalog.vbat = (*air_r2).vbat;
        peers[id].fcanalog.mAhDrawn = (*air_r2).mah;
        peers[id].fcanalog.rssi = (*air_r2).rssi;

    }
*/    
    else { // Type 0 packet (GPS + heading)

        peers[id].gps.lat = air_0.lat * 100; // From XX.12345 to XX.1234500
        peers[id].gps.lon = air_0.lon * 100; // From XX.12345 to XX.1234500
        peers[id].gps.alt = air_0.alt; // m
        peers[id].gps.groundCourse = air_0.heading * 10; // From degres to degres x 10

        if (peers[id].gps.lat != 0 && peers[id].gps.lon != 0) { // Save the last known coordinates
            peers[id].gpsrec.lat = peers[id].gps.lat;
            peers[id].gpsrec.lon = peers[id].gps.lon;
            peers[id].gpsrec.alt = peers[id].gps.alt;
            peers[id].gpsrec.groundCourse = peers[id].gps.groundCourse;
            peers[id].gpsrec.groundSpeed = peers[id].gps.groundSpeed;
        }
    }

    sys.num_peers = count_peers();

    if ((sys.air_last_received_id == curr.id) && (sys.phase > MODE_LORA_SYNC) && !sys.lora_no_tx) { // Same slot, conflict
        uint32_t cs1 = peers[id].name[0] + peers[id].name[1] * 26 + peers[id].name[2] * 26 * 26 ;
        uint32_t cs2 = curr.name[0] + curr.name[1] * 26 + curr.name[2] * 26 * 26;
        if (cs1 < cs2) { // Pick another slot
            // sprintf(sys.message, "%s", "ID CONFLICT");
            pick_id();
            resync_tx_slot(cfg.lora_timing_delay);
        }
    }
}

void lora_init() {
  // TODO Init radio;
}

// ----------------------------------------------------------------------------- Display

void display_init() {
  u8g.begin();

  u8g.setFont(DISPLAY_FONT); // u8g_font_6x10
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDrawColor(1);

  char_height = u8g.getMaxCharHeight(); // u8g.getFontAscent()-u8g.getFontDescent()+1;    
  char_width = u8g.getStrWidth("W")+1;
  screen_width = (uint8_t) u8g.getWidth();
  screen_height = (uint8_t) u8g.getHeight();  
}

void display_drawEx() {
    // display.clear();

    int j = 0;
    int line;

    if (sys.display_page == 0) {

        // display.setFont(ArialMT_Plain_24);
        // display.setTextAlignment(TEXT_ALIGN_RIGHT);
        drawVal(26, 11, curr.gps.numSat);
        drawVal(13, 42, sys.num_peers_active + 1);
        drawVal(125, 11, peer_slotname[curr.id]);

        // display.setFont(ArialMT_Plain_10);

//        display.drawString (83, 44, String(cfg.lora_cycle) + "ms");
//        display.drawString (105, 23, String(cfg.lora_nodes_max));

        // u8g.drawLine(126, 29+3, 126+16*6, 29+3);
        drawStr_F (126, 29, F("_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ "));
        drawVal (107, 44, stats.percent_received);

        //display.setTextAlignment (TEXT_ALIGN_CENTER);
        // drawCentered (64, sys.message);

        // display.setTextAlignment (TEXT_ALIGN_LEFT);
        drawStr (55, 12, curr.name);
        drawStr_F (27, 23, F("SAT"));
        drawStr_F (108, 44, F("%E"));

        drawVal_F(21, 54, sys.pps, F("p/s"));
        drawStr_F (109, 54, F("dB"));
        drawStr (55, 23, host_name[curr.host]);

        if (sys.air_last_received_id > 0) {
            drawStr (36 + sys.air_last_received_id * 8, 54, peer_slotname[sys.air_last_received_id]);
        }

        drawVal_F (15, 44, F("Nod/"), cfg.lora_nodes_max);

        if (curr.gps.fixType == 1) drawStr_F (27, 12, F("2D"));
        if (curr.gps.fixType == 2) drawStr_F  (27, 12, F("3D"));
    }

    else if (sys.display_page == 1) {

        // display.setFont (ArialMT_Plain_10);
        // display.setTextAlignment (TEXT_ALIGN_LEFT);

        u8g.drawLine(0, 11, 128, 11);

        long pos[LORA_NODES_MAX];
        long diff;

        for (int i = 0; i < LORA_NODES_MAX ; i++) {
            if (peers[i].id > 0 && !peers[i].lost) {
                diff = sys.lora_last_tx - peers[i].updated;
                if (diff > 0 && diff < cfg.lora_cycle) {
                    pos[i] = 128 - round(128 * diff / cfg.lora_cycle);
                }
            }
            else {
                pos[i] = -1;
            }
        }

        int rect_l = stats.last_tx_duration * 128 / cfg.lora_cycle;

        for (int i = 0; i < LORA_NODES_MAX; i++) {

            // display.setTextAlignment (TEXT_ALIGN_LEFT);

            if (pos[i] > -1) {
                u8g.drawFrame(pos[i], 0, rect_l, 12);
                drawStr (pos[i] + 2, 0, peer_slotname[peers[i].id]);
            }

            if (peers[i].id > 0 && j < 4) {
                line = j * 9 + 14;

                drawStr (0, line, peer_slotname[peers[i].id]);
                drawStr (12, line, peers[i].name);
                drawStr (60, line, host_name[peers[i].host]);
                // display.setTextAlignment (TEXT_ALIGN_RIGHT);

                if (peers[i].lost) { // Peer timed out
                    drawVal_F (127, line, F("L:"), (int)((sys.lora_last_tx - peers[i].updated) / 1000), F("s"));
                }
                else {
                    if (sys.lora_last_tx > peers[i].updated) {
                        drawVal (119, line, sys.lora_last_tx - peers[i].updated);
                        drawStr (127, line, "-");
                    }
                    else {
                        drawVal (119, line, cfg.lora_cycle + sys.lora_last_tx - peers[i].updated);
                        drawStr (127, line, "+");

                    }
                }
                j++;
            }
        }
    }

    else if (sys.display_page == 2) {

        const __FlashStringHelper *F_MS = F("MS");
        const __FlashStringHelper *F_SLASH = F(" / ");

        // display.setFont (ArialMT_Plain_10);
        // display.setTextAlignment (TEXT_ALIGN_LEFT);
        drawStr_F(0, 0, F("LORA TX"));
        drawStr_F(0, 10, F("MSP"));
        drawStr_F(0, 20, F("OLED"));
        drawStr_F(0, 30, F("CYCLE"));
        drawStr_F(0, 40, F("SLOTS"));
        drawStr_F(0, 50, F("UPTIME"));

        drawStr_F(112, 0, F_MS);
        drawStr_F(112, 10, F_MS);
        drawStr_F(112, 20, F_MS);
        drawStr_F(112, 30, F_MS);
        drawStr_F(112, 40, F_MS);
        drawStr_F(112, 50, F("S"));

        // display.setTextAlignment(TEXT_ALIGN_RIGHT);
        drawVal (111, 0, stats.last_tx_duration);
        drawVal_F (111, 10, stats.last_msp_duration[0], F_SLASH);
        drawVal_F (stats.last_msp_duration[1], F_SLASH);
        drawVal_F (stats.last_msp_duration[2], F_SLASH);
        drawVal (stats.last_msp_duration[3]);
        drawVal (111, 20, stats.last_oled_duration);
        drawVal (111, 30, cfg.lora_cycle);
        drawVal_F (111, 40, LORA_NODES_MAX);
        drawVal_F (F(" X "), cfg.lora_slot_spacing);
        drawVal (111, 50, ((int)millis() / 1000));

    }
    else if (sys.display_page >= 3) {

        int i = constrain(sys.display_page + 1 - LORA_NODES_MAX, 0, LORA_NODES_MAX - 1);
        bool iscurrent = (i + 1 == curr.id);

        // display.setFont(ArialMT_Plain_24);
        // display.setTextAlignment (TEXT_ALIGN_LEFT);
        drawStr (0, 0, peer_slotname[i + 1]);

        // display.setFont(ArialMT_Plain_16);
        // display.setTextAlignment(TEXT_ALIGN_RIGHT);

        if (iscurrent) {
           drawStr (128, 0, curr.name);
        }
        else {
           drawStr (128, 0, peers[i].name);
        }

        // display.setTextAlignment (TEXT_ALIGN_LEFT);
        // display.setFont (ArialMT_Plain_10);

        if (peers[i].id > 0 || iscurrent) {

            if (peers[i].lost && !iscurrent) { drawStr_F (19, 0, F("LOST")); }
                else if (peers[i].lq == 0 && !iscurrent) { drawStr (19, 0, "x"); }
                else { drawVal(19, 2, peers[i].lq); }

                if (iscurrent) {
                    drawStr_F (19, 0, F("<HOST>"));
                    drawStr (19, 12, host_name[curr.host]);
                }
                else {
                    drawStr (19, 12, host_name[peers[i].host]);
                }

/*
                if (iscurrent) {
                    drawStr (50, 12, host_state[curr.state]);
                }
                else {
                    drawStr (50, 12, host_state[peers[i].state]);
                }
*/
                // display.setTextAlignment (TEXT_ALIGN_RIGHT);

                if (iscurrent) {
                    char buf[12]; // "-123.123456" + \0;
                    dtostrf((float)curr.gps.lat / 10000000, 5, 6, buf);
                    drawVal_F (128, 24, F("LA "), buf);
                    dtostrf((float)curr.gps.lon / 10000000, 5, 6, buf);
                    drawVal_F (128, 34, F("LO "), buf);
                }
                else {
                    char buf[12]; // "-123.123456" + \0;
                    dtostrf((float)peers[i].gpsrec.lat / 10000000, 5, 6, buf);
                    drawVal_F (128, 24, F("LA "), buf);
                    dtostrf((float)peers[i].gpsrec.lon / 10000000, 5, 6, buf);
                    drawVal_F (128, 34, F("LO "), buf);
                }

                // display.setTextAlignment (TEXT_ALIGN_LEFT);

                if (iscurrent) {
                    drawVal_F (0, 24, F("A "), curr.gps.alt, F("m"));
                    drawVal_F (0, 34, F("S "), peers[i].gpsrec.groundSpeed / 100, F("m/s"));
                    drawVal_F (0, 44, F("C "), curr.gps.groundCourse / 10, F("°"));
                }
                else {
                    drawVal_F (0, 24, F("A "), peers[i].gpsrec.alt, F("m"));
                    drawVal_F (0, 34, F("S "), peers[i].gpsrec.groundSpeed / 100, F("m/s"));
                    drawVal_F (0, 44, F("C "), peers[i].gpsrec.groundCourse / 10, F("°"));
                }

                if (peers[i].gps.lat != 0 && peers[i].gps.lon != 0 && curr.gps.lat != 0 && curr.gps.lon != 0 && !iscurrent) {


                    double lat1 = curr.gps.lat / 10000000;
                    double lon1 = curr.gps.lon / 10000000;
                    double lat2 = peers[i].gpsrec.lat / 10000000;
                    double lon2 = peers[i].gpsrec.lon / 10000000;

                    peers[i].distance = gpsDistanceBetween(lat1, lon1, lat2, lon2);
                    peers[i].direction = gpsCourseTo(lat1, lon1, lat2, lon2);
                    peers[i].relalt = peers[i].gpsrec.alt - curr.gps.alt;

                    drawVal_F (40, 44, F("B "), peers[i].direction, F("°"));
                    drawVal_F (88, 44, F("D "), peers[i].distance, F("m"));
                    drawVal_F (0, 54, F("R "), peers[i].relalt, F("m"));
                }
/*
                if (iscurrent) {
                    drawVal_F (40, 54, (float)curr.fcanalog.vbat / 10, F("v"));
                    drawVal_F (88, 54, (int)curr.fcanalog.mAhDrawn, F("mah"));
                }
                else {
                    drawVal_F (40, 54, (float)peers[i].fcanalog.vbat / 10, F("v"));
                    drawVal_F (88, 54, (int)peers[i].fcanalog.mAhDrawn, F("mah"));
                }
*/
            // display.setTextAlignment (TEXT_ALIGN_RIGHT);

        }
        else {
            drawStr_F (35, 7, F("SLOT IS EMPTY"));
        }

    }

    sys.air_last_received_id = 0;
    // sys.message[0] = 0;
    // display.display();
}

void display_draw() {
  u8g.firstPage();
  do  {
    display_drawEx();
  } while( u8g.nextPage() );
}

void display_logo() {
  u8g.firstPage();
  do  {
    drawCentered_F(0, F("iNAV Radar for HC-12"));
    drawCentered_F(char_height*2, F(VERSION));
    drawCentered_F(char_height*3, F("(c) by Andrey Prikupets"));
    drawCentered_F(char_height*4, F("based on INAV RADAR"));
  } while( u8g.nextPage() );
  
  delay(1200);
}

// -------- MSP and FC

/*
void msp_get_state() {
    uint32_t planeModes;
    msp.getActiveModes(&planeModes);
    curr.state = bitRead(planeModes, 0);
}
*/

void msp_get_name() {
    msp.request(MSP_NAME, &curr.name, sizeof(curr.name));
    curr.name[LORA_NAME_LENGTH] = '\0'; // 6
}

void msp_get_gps() {
    msp.request(MSP_RAW_GPS, &curr.gps, sizeof(curr.gps));
}

void msp_set_fc() {
    char j[5];
    curr.host = HOST_NONE;
    msp.request(MSP_FC_VARIANT, &j, sizeof(j));

    if (strncmp(j, "INAV", 4) == 0) {
        curr.host = HOST_INAV;
    }
    else if (strncmp(j, "BTFL", 4) == 0) {
        curr.host = HOST_BTFL;
    }

    if (curr.host == HOST_INAV || curr.host == HOST_BTFL) {
        msp.request(MSP_FC_VERSION, &curr.fcversion, sizeof(curr.fcversion));
    }
 }

/*
void msp_get_fcanalog() {
  msp.request(MSP_ANALOG, &curr.fcanalog, sizeof(curr.fcanalog));
}
*/

void msp_send_radar(uint8_t i) {
    radarPos.id = i;
    radarPos.state = 0; // peers[i].state;
    radarPos.lat = peers[i].gps.lat; // x 10E7
    radarPos.lon = peers[i].gps.lon; // x 10E7
    radarPos.alt = peers[i].gps.alt * 100; // cm
    radarPos.heading = peers[i].gps.groundCourse / 10; // From ° x 10 to °
    radarPos.speed = peers[i].gps.groundSpeed; // cm/s
    radarPos.lq = peers[i].lq;
    msp.command2(MSP2_COMMON_SET_RADAR_POS , &radarPos, sizeof(radarPos), 0);
//    msp.command(MSP_SET_RADAR_POS , &radarPos, sizeof(radarPos), 0);
}

void msp_send_peers() {
    for (int i = 0; i < LORA_NODES_MAX; i++) {
        if (peers[i].id > 0) {
            msp_send_radar(i);
        }
    }
}

void msp_send_peer(uint8_t peer_id) {
    if (peers[peer_id].id > 0) {
        msp_send_radar(peer_id);
    }
}

// -------- INTERRUPTS

void buttonChanged(int state){
    sys.io_button_pressed = !state;
    if (sys.io_button_pressed == 1) {

        if (sys.display_page >= 3 + LORA_NODES_MAX) {
            sys.display_page = 0;
        }
        else {
            sys.display_page++;
        }
        if (sys.num_peers == 0 && sys.display_page == 1)  { // No need for timings graphs when alone
            sys.display_page++;
        }
    }
}

// ----------------------------- setup

void setup() {
    button.setCallback(buttonChanged);
    
    set_mode();

    pinMode(PIN_HC12_SET, OUTPUT);
    digitalWrite(PIN_HC12_SET, HIGH); // 1=data mode; 0=AT commands mode;
    HC12.begin(HC12_BAUD_RATE);
    hc12.begin(HC12);

    display_init();
    display_logo();

    lora_init();

    msp.begin(Serial);
    Serial.begin(MSP_BAUD_RATE);

    reset_peers();

//    display.drawString (0, 9, "HOST");
//    display.display();

    sys.display_updated = 0;
    sys.cycle_scan_begin = millis();

    sys.io_led_blink = 0;

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    curr.host = HOST_NONE;

    sys.phase = MODE_HOST_SCAN;
}

// ----------------------------------------------------------------------------- MAIN LOOP

void loop() {
    button.update();
    
    sys.now = millis();

// ---------------------- HOST SCAN

    if (sys.phase == MODE_HOST_SCAN) {
            if ((sys.now > (sys.cycle_scan_begin + cfg.msp_fc_timeout)) || (curr.host != HOST_NONE)) {  // End of the host scan

            if (curr.host != HOST_NONE) {
                msp_get_name();
            }

            if (curr.name[0] == '\0') {
                for (int i = 0; i < 4; i++) {
                curr.name[i] = (char) random(65, 90);
                curr.name[4] = 0;
                }
            }

            curr.gps.fixType = 0;
            curr.gps.lat = 0;
            curr.gps.lon = 0;
            curr.gps.alt = 0;
            curr.id = 0;
            
            u8g.firstPage();
            do  {
              drawStr (35, 9, host_name[curr.host]);
              if (curr.host > 0) {
                drawStr(" ");
                drawVal(curr.fcversion.versionMajor);
                drawStr(".");
                drawVal(curr.fcversion.versionMinor);
                drawStr(".");
                drawVal(curr.fcversion.versionPatchLevel);
              }

              drawProgressBar(0, 53, 40, 6, 100);
              drawStr_F (0, 18, F("SCAN"));
              // display.display();
            } while( u8g.nextPage() );


            sys.cycle_scan_begin = millis();
            sys.phase = MODE_LORA_INIT;

        } else { // Still scanning
            if ((sys.now > sys.display_updated + cfg.cycle_display / 2) ) { // && sys.display_enable

                delay(50);
                msp_set_fc();

                u8g.firstPage();
                do  {
                  drawProgressBar(0, 53, 40, 6, 100 * (millis() - sys.cycle_scan_begin) / cfg.msp_fc_timeout);
                  // display.display();
                } while( u8g.nextPage() );
                
                sys.display_updated = millis();
            }
        }
    }

// ---------------------- LORA INIT

    if (sys.phase == MODE_LORA_INIT) {
        if (sys.now > (sys.cycle_scan_begin + cfg.cycle_scan)) {  // End of the scan, set the ID then sync

            sys.num_peers = count_peers();

            if (sys.num_peers >= LORA_NODES_MAX || sys.io_button_pressed) { // || sys.io_button_released > 0
                sys.lora_no_tx = 1;
                sys.display_page = 0;
            }
            else {
//                cfg.lora_cycle =  cfg.lora_slot_spacing * cfg.lora_air_mode;
                pick_id();
            }

            sys.phase = MODE_LORA_SYNC;

        } else { // Still scanning
            if ((sys.now > sys.display_updated + cfg.cycle_display / 2) ) { // && sys.display_enable
                u8g.firstPage();
                do  {
                  for (int i = 0; i < LORA_NODES_MAX; i++) {
                    if (peers[i].id > 0) {
                        drawStr(40 + peers[i].id * 8, 18, peer_slotname[peers[i].id]);
                    }
                  }
                  drawProgressBar(40, 53, 86, 6, 100 * (millis() - sys.cycle_scan_begin) / cfg.cycle_scan);
                  // display.display();
                } while( u8g.nextPage() );
                sys.display_updated = millis();
            }
        }
    }

// ---------------------- LORA SYNC

    if (sys.phase == MODE_LORA_SYNC) {

        if (sys.num_peers == 0 || sys.lora_no_tx) { // Alone or no_tx mode, start at will
            sys.lora_next_tx = millis() + cfg.lora_cycle;
            }
        else { // Not alone, sync by slot
            resync_tx_slot(cfg.lora_timing_delay);
        }
        sys.display_updated = sys.lora_next_tx + cfg.lora_cycle - 30;
        sys.stats_updated = sys.lora_next_tx + cfg.lora_cycle - 15;

        sys.pps = 0;
        sys.ppsc = 0;
        sys.num_peers = 0;
        stats.packets_total = 0;
        stats.packets_received = 0;
        stats.percent_received = 0;

        digitalWrite(PIN_LED, LOW);

        sys.phase = MODE_LORA_RX;
        }

// ---------------------- LORA RX

    if ((sys.phase == MODE_LORA_RX) && (sys.now > sys.lora_next_tx)) {

        // sys.lora_last_tx = sys.lora_next_tx;

        while (sys.now > sys.lora_next_tx) { // In  case we skipped some beats
            sys.lora_next_tx += cfg.lora_cycle;
        }

        if (sys.lora_no_tx) {
            // sprintf(sys.message, "%s", "SILENT MODE (NO TX)");
        }
        else {
            sys.phase = MODE_LORA_TX;
        }

    sys.lora_tick++;

    }

// ---------------------- LORA TX

    if (sys.phase == MODE_LORA_TX) {

        if ((curr.host == HOST_NONE) || (curr.gps.fixType < 1)) {
            curr.gps.lat = 0;
            curr.gps.lon = 0;
            curr.gps.alt = 0;
            curr.gps.groundCourse = 0;
            curr.gps.groundSpeed = 0;
        }

        sys.lora_last_tx = millis();
        lora_send();
        stats.last_tx_duration = millis() - sys.lora_last_tx;

        // Drift correction

        if (curr.id > 1) {
            int prev = curr.id - 2;
            if (peers[prev].id > 0) {
                sys.lora_drift = sys.lora_last_tx - peers[prev].updated - cfg.lora_slot_spacing;

                if ((abs(sys.lora_drift) > cfg.lora_antidrift_threshold) && (abs(sys.lora_drift) < (cfg.lora_slot_spacing * 0.5))) {
                    sys.drift_correction = constrain(sys.lora_drift, -cfg.lora_antidrift_correction, cfg.lora_antidrift_correction);
                    sys.lora_next_tx -= sys.drift_correction;
                    // sprintf(sys.message, "%s %3d", "TIMING ADJUST", -sys.drift_correction);
                }
            }
        }

        sys.lora_slot = 0;
        sys.msp_next_cycle = sys.lora_last_tx + cfg.msp_after_tx_delay;

        // Back to RX
        sys.phase = MODE_LORA_RX;
    }

// ---------------------- DISPLAY

    if ((sys.now > sys.display_updated + cfg.cycle_display) && (sys.phase > MODE_LORA_SYNC)) { // && sys.display_enable 

        stats.timer_begin = millis();
        display_draw();
        stats.last_oled_duration = millis() - stats.timer_begin;
        sys.display_updated = sys.now;
    }

// ---------------------- SERIAL / MSP

    if (sys.now > sys.msp_next_cycle && curr.host != HOST_NONE && sys.phase > MODE_LORA_SYNC && sys.lora_slot < LORA_NODES_MAX) {

        stats.timer_begin = millis();

/* removed to save space;
        if (sys.lora_slot == 0) {

            if (sys.lora_tick % 6 == 0) {
                msp_get_state();
            }

            if ((sys.lora_tick + 1) % 6 == 0) {
                msp_get_fcanalog();
            }

        }
*/
        msp_get_gps(); // GPS > FC > ESP
        msp_send_peer(sys.lora_slot); // ESP > FC > OSD

        stats.last_msp_duration[sys.lora_slot] = millis() - stats.timer_begin;
        sys.msp_next_cycle += cfg.lora_slot_spacing;
        sys.lora_slot++;

    }


// ---------------------- STATISTICS & IO

    if ((sys.now > (cfg.cycle_stats + sys.stats_updated)) && (sys.phase > MODE_LORA_SYNC)) {

        sys.pps = sys.ppsc;
        sys.ppsc = 0;

        // Timed-out peers + LQ

        for (int i = 0; i < LORA_NODES_MAX; i++) {

            if (sys.now > (peers[i].lq_updated +  cfg.lora_cycle * 4)) {
                uint16_t diff = peers[i].updated - peers[i].lq_updated;
                peers[i].lq = constrain(peers[i].lq_tick * 4.4 * cfg.lora_cycle / diff, 0, 4);
                peers[i].lq_updated = sys.now;
                peers[i].lq_tick = 0;
            }

            if (peers[i].id > 0 && ((sys.now - peers[i].updated) > cfg.lora_peer_timeout)) {
                peers[i].lost = 1;
            }

        }

        sys.num_peers_active = count_peers(1);
        stats.packets_total += sys.num_peers_active * cfg.cycle_stats / cfg.lora_cycle;
        stats.packets_received += sys.pps;
        stats.percent_received = (stats.packets_received > 0) ? constrain(100 * stats.packets_received / stats.packets_total, 0 ,100) : 0;

/*
        if (sys.num_peers >= (cfg.lora_air_mode - 1)&& (cfg.lora_air_mode < LORA_NODES_MAX)) {
            cfg.lora_air_mode++;
            sys.lora_next_tx += cfg.lora_slot_spacing ;
            cfg.lora_cycle =  cfg.lora_slot_spacing * cfg.lora_air_mode;
            }
 */

        // Screen management

/*
        if (!curr.state && !sys.display_enable) { // Aircraft is disarmed = Turning on the OLED
            // display.displayOn();
            sys.display_enable = 1;
        }

        else if (curr.state && sys.display_enable) { // Aircraft is armed = Turning off the OLED
            // display.displayOff();
            sys.display_enable = 0;
        }
*/
    sys.stats_updated = sys.now;
    }


    // LED blinker

    if (sys.lora_tick % 6 == 0) {
        if (sys.num_peers_active > 0) {
            sys.io_led_changestate = millis() + IO_LEDBLINK_DURATION;
            sys.io_led_count = 0;
            sys.io_led_blink = 1;
        }
    }

    if (sys.io_led_blink && millis() > sys.io_led_changestate) {

        sys.io_led_count++;
        sys.io_led_changestate += IO_LEDBLINK_DURATION;

        if (sys.io_led_count % 2 == 0) {
            digitalWrite(PIN_LED, LOW);
        }
        else {
            digitalWrite(PIN_LED, HIGH);
        }

        if (sys.io_led_count >= sys.num_peers_active * 2) {
            sys.io_led_blink = 0;
        }

    }

    if(hc12.available()) {
        lora_receive();
    }

}
