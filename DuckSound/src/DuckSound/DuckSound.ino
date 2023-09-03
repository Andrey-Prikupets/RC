#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <RCReceive.h>
#include <Random16.h>

#define VOLUME 10
#define DELAY_MS 1500

#define RX_SET_1 1250 // below that sound is off; 
#define RX_SET_2 1750 // below that it playes Set 1, above that - Set 2;

#define SET_1_FILES 20 // Files since 21th belong to Set 2;

#define DFPLAYER_RX_PIN 10 // Connect to DFPlayer's TX;
#define DFPLAYER_TX_PIN 11 // Connect to DFPlayer's RX;

#define RX_CHANNEL_1_PIN 2 // Arduino Uno, Nano, Mini, other 328-based: pins 2 and 3 are available;

#define LED_PIN LED_BUILTIN

SoftwareSerial mySoftwareSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN); // RX, TX
DFRobotDFPlayerMini player;
RCReceive rcReceiver;
Random16 rnd;

int filesCount;

enum States {OFF, DELAYING, READY_TO_PLAY, PLAYING};
enum States state;

enum Set {SET_NONE = 0, SET_1 = 1, SET_2 = 2};
enum Set set;

unsigned long timer;

void setup()
{
  delay(500);
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!player.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert or replace the SD card!"));
    while(true){
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  
  Serial.print(F("Folders: ")); Serial.println(player.readFolderCounts()); 
  Serial.print(F("Files: ")); Serial.println(player.readFileCounts()); 

  filesCount = player.readFileCountsInFolder(1);
  Serial.print(F("Files in 01 folder: ")); Serial.println(filesCount); 

  Serial.print(F("Set 1: 1..")); Serial.println(SET_1_FILES); 
  Serial.print(F("Set 2: ")); Serial.print(SET_1_FILES+1); 
  Serial.print(F("..")); Serial.println(filesCount); 

  Serial.print(F("Volume: ")); Serial.println(VOLUME); 

  player.volume(VOLUME);  //Set volume value. From 0 to 30
  player.disableDAC();

  state = OFF;
  set = SET_NONE;

  rcReceiver.attachInt(RX_CHANNEL_1_PIN);

  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  delay(200);
}

void printDetail(uint8_t type, int value);

int rc_value;

Set getSet() {
  if (rcReceiver.hasNP() && !rcReceiver.hasError()) {
    rc_value = rcReceiver.getMsValue();
    // Serial.print(F("RX: ")); Serial.print(rc_value);
    
    if (rc_value < RX_SET_1) {
      // Serial.println(F(" NONE"));
      return SET_NONE;
    } else
    if (rc_value < RX_SET_2) {
      // Serial.println(F(" SET_1"));
      return SET_1;
    } else {
      // Serial.println(F(" SET_2"));
      return SET_2;
    }
  } else {
    // Errors happen too offten, and it is better lock previous value instead of stopping playing;
    // Serial.println(F("Error"));
    // return SET_NONE;
    return set;
  }
}

int getRandom(int from, int to) { 
  return (rnd.get() % (to-from+1))+from;
}

void loop()
{
  Set _set = getSet();
  if (_set != set) {
    Serial.print(F("RX: ")); Serial.println(rc_value);
    set = _set;  
    if (set == SET_NONE) {
      state = OFF;
      Serial.println(F("Stop playing"));
      player.stop();
      player.disableDAC();
      digitalWrite(LED_PIN, LOW);
    } else {
      rnd.setSeed((uint16_t) millis());
      Serial.print(F("Start playing Set ")); Serial.println(set);
      player.stop();
      state = READY_TO_PLAY;
    }
  }
  
  if (player.available()) {
    if (player.readType() == DFPlayerPlayFinished) {
      Serial.print(F("Playing finished for ")); Serial.println(player.read());
      if (state == PLAYING) {
        player.disableDAC();
        state = DELAYING;
        timer = millis()+DELAY_MS;
        Serial.print(F("Delaying ")); Serial.println(DELAY_MS);
        digitalWrite(LED_PIN, LOW);
      }
    } else {
      printDetail(player.readType(), player.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
  }

  if (state == DELAYING) {
    if (millis() > timer) {
       Serial.println(F("Delay finished"));
       state = READY_TO_PLAY;
    }
  }

  if (state == READY_TO_PLAY) {
     player.enableDAC();
     int n = set == SET_1 ? getRandom(1, SET_1_FILES) : getRandom(SET_1_FILES+1, filesCount);
     Serial.print(F("Play ")); Serial.print(n); Serial.print(F(", set ")); Serial.println(set);
     player.playFolder(1, n);  //play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
     state = PLAYING;
     digitalWrite(LED_PIN, HIGH);
  }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}
