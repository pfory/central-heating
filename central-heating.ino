/*
--------------------------------------------------------------------------------------------------------------------------

               CENTRAL HEATING - control system for central heating

Petr Fory pfory@seznam.cz

Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating

Version history:
0.5             - i2c keyboard
0.4 - 23.2.2016 - add RTC, prenos teploty na satelit
0.3 - 16.1.2015

compilated by Arduino 1.6.4

--------------------------------------------------------------------------------------------------------------------------
HW
Pro Mini 328
I2C display
1 Relays module
DALLAS
keyboard

Pro Mini 328 Layout
------------------------------------------
A0              - 
A1              - 
A2              - RX from CommUnit
A3              - TX to CommUnit
A4              - I2C display SDA 0x27, I2C Central heating unit 0x02, keypad 0x20, 
A5              - I2C display SCL 0x27, I2C Central heating unit 0x02, keypad 0x20
D0              - Rx
D1              - Tx
D2              - DALLAS
D3              - 
D4              - DALLAS
D5              - DALLAS
D6              - RELAY
D7              - 
D8              - LED
D9              - 
D10             - 
D11             - 
D12             - 
D13             - BUZZER
--------------------------------------------------------------------------------------------------------------------------
*/

#define watchdog //enable this only on board with optiboot bootloader
#ifdef watchdog
#include <avr/wdt.h>
#endif


#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "beep.h"
#include <EEPROM.h>
#include <avr/wdt.h>

//wiring
#define ONE_WIRE_BUS_IN                     2
#define ONE_WIRE_BUS_OUT                    4
#define ONE_WIRE_BUS_UT                     5
#define RELAYPIN                            6
#define LEDPIN                              8
#define BUZZERPIN                           13

Beep beep(BUZZERPIN);

#define IN                                  0
#define OUT                                 1
#define RELAY                               100
#define RELAYTEMP                           101

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWireOUT(ONE_WIRE_BUS_OUT);
OneWire oneWireIN(ONE_WIRE_BUS_IN);
OneWire oneWireUT(ONE_WIRE_BUS_UT);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsOUT(&oneWireOUT);
DallasTemperature sensorsIN(&oneWireIN);
DallasTemperature sensorsUT(&oneWireUT);

DeviceAddress inThermometer, outThermometer;
#define TEMPERATURE_PRECISION 12


const unsigned long   measDelay                = 10000; //in ms
unsigned long         lastMeas                 = measDelay * -1;
const unsigned long   measTime                 = 750; //in ms
const unsigned long   sendDelay                = 20000; //in ms
unsigned long         lastSend                 = sendDelay * -1;
bool                  startConversion          = false;
unsigned long         startConversionMillis    = 0;
float                 tempOUT                  = 0.f;
float                 tempIN                   = 0.f;
float                 tempUT[12];           
bool                  relay                    = HIGH;
const unsigned long   pumpProtect              = 864000000;  //1000*60*60*24*10; //in ms = 10 day, max 49 days
const unsigned long   pumpProtectRun           = 300000;     //1000*60*5;     //in ms = 5 min
bool                  firstMeasComplete        = false;
#define TEMP_ERR -127     
                    
unsigned long const SERIAL_SPEED               = 115200;  //kvuli BT modulu 9600 jinak muze byt vice
unsigned long const COMM_SPEED                 = 9600;

#define DS1307

#ifdef DS1307
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeLib.h>
#define time
#endif


#ifdef time
#include <Time.h>
#include <Streaming.h>        
#include <Time.h>             
bool parse=false;
bool config=false;
tmElements_t    tm;
const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
bool isTime = true;
#endif

#define CONFIG_START 0
#define CONFIG_VERSION "v08"

struct StoreStruct {
  // This is for mere detection if they are your settings
  char            version[4];
  // The variables of your settings
  unsigned int    moduleId;  // module id
  bool            relay;     // relay state
  float           tempON;
  float           tempOFFDiff;
  float           tempAlarm;
#ifdef time
  tmElements_t    lastPumpRun;
#endif
} storage = {
  CONFIG_VERSION,
  // The default module 0
  0,
  0, // off
  65,
  2,
  95
};

#include <LiquidCrystal_I2C.h>
#define LCDADDRESS   0x27
#define EN           2
#define RW           1
#define RS           0
#define D4           4
#define D5           5
#define D6           6
#define D7           7
#define BACKLIGHT    3
#define POL          POSITIVE
#define LCDROWS      4
#define LCDCOLS      20
LiquidCrystal_I2C lcd(LCDADDRESS,EN,RW,RS,D4,D5,D6,D7,BACKLIGHT,POL);  // set the LCD

#include <SoftwareSerial.h>
#define RX A2
#define TX A3
SoftwareSerial mySerial(RX, TX);
#define serial

#include <avr/pgmspace.h>
unsigned long crc;
const PROGMEM uint32_t crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};
#define START_BLOCK       '#'
#define DELIMITER         ';'
#define END_BLOCK         '$'
#define END_TRANSMITION   '*'

#define keypad
#ifdef keypad
int address                               = 0x20;
uint8_t data;
int error;
uint8_t pin                               = 0;
char key                                  = ' ';
char keyOld                               = ' ';
unsigned int repeatAfterMs                = 300;
//unsigned int repeatCharSec                = 10;
unsigned long lastKeyPressed              = 0;

//define the symbols on the buttons of the keypads
const byte ROWS                           = 4; //four rows
const byte COLS                           = 4; //four columns
char hexaKeys[ROWS][COLS]                 = {
                                            {'*','7','4','1'},
                                            {'0','8','5','2'},
                                            {'#','9','6','3'},
                                            {'D','C','B','A'}
};
byte displayVar=1;
byte displayVarSub=1;
#endif



//SW name & version
float const   versionSW                   = 0.61;
char  const   versionSWString[]           = "Central heat v"; 

const byte STATUS_AFTER_BOOT  = 9;

/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
  Wire.begin();
  Serial.begin(SERIAL_SPEED);
  Serial.print(versionSWString);
  Serial.println(versionSW);
  beep.Delay(100,40,1,255);
  
  lcd.begin(LCDCOLS,LCDROWS);               // initialize the lcd 
  lcd.setBacklight(255);
  lcd.clear();
  lcd.print(versionSWString);
  lcd.print(versionSW);
  mySerial.begin(COMM_SPEED);
  
#ifdef DS1307
  if (RTC.read(tm)) {
    Serial.print("RTC OK, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    //Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(tm.Year);
    Serial.println();
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
  }
#endif

#ifdef time
  // Setup time library  
  Serial.print("RTC Sync");
  setSyncProvider(RTC.get);          // the function to get the time from the RTC
  if(timeStatus() == timeSet) {
    Serial.println(" OK!");
  } else {
    Serial.println(" FAIL!");
  }
  //printDateTime();
#endif
  loadConfig();
  Serial.print("Temp ON ");
  Serial.println(storage.tempON);
  Serial.print("Temp OFF diff ");
  Serial.println(storage.tempOFFDiff);
  Serial.print("Temp alarm ");
  Serial.println(storage.tempAlarm);
 
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,!relay);
  pinMode(BUZZERPIN, OUTPUT);

  delay(1000);
  lcd.clear();
  while (true) {
    sensorsOUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
    sensorsIN.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
    sensorsUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

    if (sensorsIN.getDeviceCount()==0 || sensorsOUT.getDeviceCount()==0) {
      beep.Delay(100,40,1,255);
      Serial.println("NO temperature sensor(s) DS18B20 found!!!!!!!!!");
      lcd.setCursor(0, 1);
      lcd.print("!NO temp.sensor(s)!!");
      lcd.setCursor(0, 2);
      lcd.print("!!!DS18B20 found!!!!");
      lcd.setCursor(0, 3);
      lcd.print("!!!!!Check wire!!!!!");
#ifdef time
      displayTime();
      //break;
#endif
      } else {
      break;
    }
    delay(800);
  }

  sensorsOUT.setResolution(TEMPERATURE_PRECISION);
  sensorsOUT.setWaitForConversion(false);
  sensorsIN.setResolution(TEMPERATURE_PRECISION);
  sensorsIN.setWaitForConversion(false);
  sensorsUT.setResolution(TEMPERATURE_PRECISION);
  sensorsUT.setWaitForConversion(false);

  
  Serial.println();
  Serial.print("Sensor(s) ");
  Serial.print(sensorsIN.getDeviceCount());
  Serial.print(" on bus IN - pin ");
  Serial.println(ONE_WIRE_BUS_IN);
  Serial.print("Device Address: ");
  sensors.getAddress(inThermometer, 0); 
  printAddress(inThermometer);
  Serial.println();

  lcd.setCursor(0,1);
  lcd.print("Sen.");
  lcd.print(sensorsIN.getDeviceCount());
  lcd.print(" bus IN");

  Serial.print("Sensor(s) ");
  Serial.print(sensorsOUT.getDeviceCount());
  Serial.print(" on bus OUT - pin ");
  Serial.println(ONE_WIRE_BUS_OUT);
  Serial.print("Device Address: ");
  sensors.getAddress(outThermometer, 0); 
  printAddress(outThermometer);
  Serial.println();
   
  lcd.setCursor(0,2);
  lcd.print("Sen.");
  lcd.print(sensorsOUT.getDeviceCount());
  lcd.print(" bus OUT");
 
  Serial.print("Sensor(s) ");
  Serial.print(sensorsUT.getDeviceCount());
  Serial.print(" on bus UT - pin ");
  Serial.println(ONE_WIRE_BUS_UT);
  
  lcd.setCursor(0,3);
  lcd.print("Sen.");
  lcd.print(sensorsUT.getDeviceCount());
  lcd.print(" bus UT");

  
  
  Serial.print("Send interval ");
  Serial.print(sendDelay);
  Serial.println(" sec");
  
  delay(1000);
  lcd.clear();
  
  displayInfo();

  delay(1000);
  lcd.clear();

#ifdef watchdog
  wdt_enable(WDTO_8S);
#endif
  
  lastSend=0;
  //lastMeas=-measDelay;
  Serial.println("Setup end.");
}

//bool first=true;
/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
/*  if (first) {
    beep.noDelay(100,40,4,255);
    first=false;
  }
*/  
#ifdef watchdog
  wdt_reset();
#endif  
#ifdef time
  //setTime();
#endif  
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    startMeas(); 
    startConversion=true;
    startConversionMillis = millis();
  }
  if (startConversion && (millis() - startConversionMillis >= measTime)) {
    startConversion=false;
    getTemp();
    printTemp();
    //poslani teploty do LED displeje
    Wire.beginTransmission(8);
    //tempOUT=random(0,100);
    Wire.write((byte)tempOUT);
    Wire.endTransmission();

    if (tempOUT <= storage.tempON - storage.tempOFFDiff) {
      relay = HIGH;
#ifdef time
      storage.lastPumpRun = tm;
#endif
    }
    if ((tempOUT >= storage.tempON) || (tempIN >= storage.tempON)) {
      relay = LOW;
    }

    if (relay == LOW) {
      Serial.println("Relay ON");
    } else {
      Serial.println("Relay OFF");
    }

    digitalWrite(RELAYPIN,!relay);
    digitalWrite(LEDPIN,relay);
  }
  if (millis() - lastSend >= sendDelay) {
    sendDataSerial();
    lastSend = millis();
  }

  if (tempOUT >= storage.tempAlarm) {
    beep.noDelay(100,40,3,255);
  }
  /*    if ((tempOUT || tempIN) >= storage.tempAlarm && (tempOUT || tempIN) < storage.tempAlarm+5) {
    beep.noDelay(100,40,3,255);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+5 && (tempOUT || tempIN) < storage.tempAlarm+10) {
    beep.noDelay(100,40,5,255);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+10) {
    beep.noDelay(100,40,9,255);
  } */

  display();
  
  beep.loop();
  
  keyPressed();
  keyBoard();
  
#ifdef time
  testPumpProtect();
#endif
}

/////////////////////////////////////////////   F  U  N  C   ///////////////////////////////////////
void startMeas() {
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensorsIN.requestTemperatures(); // Send the command to get temperatures
  sensorsOUT.requestTemperatures(); // Send the command to get temperatures
  sensorsUT.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
}

void getTemp() {
  float tempUTRaw[sensorsUT.getDeviceCount()];
  tempIN = sensorsIN.getTempCByIndex(0);
  tempOUT = sensorsOUT.getTempCByIndex(0);
  for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    tempUTRaw[i]=sensorsUT.getTempCByIndex(i);
  }
  
    //remaping temp
  //Rad1 - LivingRoom 
  //Rad2 - BedroomNew
  //Rad3 - WorkRoom
  //Rad4 - BedRoomOld
  //Rad5 - Bojler
  //Rad6 - 
  //Rad7 -
  //Rad8 - 

  tempUT[0]=tempUTRaw[5];
  tempUT[1]=tempUTRaw[4];
  tempUT[2]=tempUTRaw[3];
  tempUT[3]=tempUTRaw[7];
  tempUT[4]=tempUTRaw[0];
  tempUT[5]=tempUTRaw[1];
  tempUT[6]=tempUTRaw[2];
  tempUT[7]=tempUTRaw[6];
  tempUT[8]=tempUTRaw[7];
  tempUT[9]=tempUTRaw[8];

  firstMeasComplete=true;
  Serial.println(millis());
}

void printTemp() {
  Serial.print("Temp IN: ");
  Serial.print(tempIN); 
  Serial.println();
  Serial.print("Temp OUT: ");
  Serial.print(tempOUT); 
  Serial.println();
  for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    Serial.print("Temp UT[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.print(tempUT[i]); 
    Serial.println();
  }
}

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
    for (unsigned int t=0; t<sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(storage); t++) {
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
  }
}

//display
/*
  01234567890123456789
  --------------------
0|xx/xx       15:30:45
1|xx/xx  xx/xx  xx/xx
2|xx/xx  xx/xx  xx/xx
3|xx/xx  xx/xx  xx/xx
  --------------------
  01234567890123456789
*/

void displayTemp() {
#ifdef time
  displayTime();
#endif

  lcd.setCursor(0, 0); //col,row
  lcd.print("       ");
  lcd.setCursor(0, 0); //col,row
  if (tempIN==TEMP_ERR) {
    displayTempErr();
  }else {
    addSpaces((int)tempIN);
    lcd.print((int)tempIN);
  }
  lcd.setCursor(2, 0); //col,row
  lcd.print("/");
  //addSpaces((int)tempOUT);
  if (tempOUT==TEMP_ERR) {
    displayTempErr();
  }else {
    lcd.print((int)tempOUT);
  }
  
  byte radka=1;
  byte sensor=0;
  byte radiator=1;
  for (byte i=0; i<sensorsUT.getDeviceCount(); i=i+6) {
    lcd.setCursor(0, radka);
    //lcd.print(radiator++);
    //lcd.print(":        ");
    //lcd.setCursor(2, radka);
    if (tempUT[sensor]==TEMP_ERR) {
      displayTempErr();
    }else {
      addSpaces((int)tempUT[sensor]);
      lcd.print((int)tempUT[sensor]);
    }
    lcd.setCursor(2, radka);
    lcd.print("/");
    if (tempUT[++sensor]==TEMP_ERR) {
      displayTempErr();
    }else {
      //addSpaces((int)tempUT[sensor]);
      lcd.print((int)tempUT[sensor]);    
    }

    lcd.setCursor(7, radka);
    //lcd.print(radiator++);
    //lcd.print(":        ");
    //lcd.setCursor(12, radka);
    if (tempUT[sensor]==TEMP_ERR) {
      displayTempErr();
    }else {
      addSpaces((int)tempUT[++sensor]);
      lcd.print((int)tempUT[sensor]);    
    }
    lcd.setCursor(9, radka);
    lcd.print("/");
    if (tempUT[++sensor]==TEMP_ERR) {
      displayTempErr();
    }else {
      //addSpaces((int)tempUT[++sensor]);
      lcd.print((int)tempUT[sensor++]);    
    }

    lcd.setCursor(14, radka);
    //lcd.print(radiator++);
    //lcd.print(":        ");
    //lcd.setCursor(12, radka);
    if (tempUT[sensor]==TEMP_ERR) {
      displayTempErr();
    }else {
      addSpaces((int)tempUT[++sensor]);
      lcd.print((int)tempUT[sensor]);    
    }
    lcd.setCursor(16, radka++);
    lcd.print("/");
    if (tempUT[++sensor]==TEMP_ERR) {
      displayTempErr();
    }else {
      //addSpaces((int)tempUT[++sensor]);
      lcd.print((int)tempUT[sensor++]);    
    }
  }

  lcd.setCursor(8, 0);
  if (relay==HIGH) {
    lcd.print("    ");
  }else{
    lcd.print("CER");
  }
}

void displayTempErr() {
  lcd.print("Er");
}


void addSpaces(int cislo) {
  //if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  if (cislo<=0 && cislo>-10) lcd.print(" ");
}

#ifdef time
//display time on LCD
void lcd2digits(int number) {
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}


void displayTime() {
  lcd.setCursor(12, 0); //col,row
  if (RTC.read(tm)) {
    lcd2digits(tm.Hour);
    lcd.write(':');
    lcd2digits(tm.Minute);
    lcd.write(':');
    lcd2digits(tm.Second);
  } else {
    lcd.write('        ');
  }
}

//zabranuje zatuhnuti cerpadla v lete
void testPumpProtect() {
  //if (storage.lastPumpRun
}

void setTime() {
  static time_t tLast;
  time_t t;
  
  tmElements_t tm;
  //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  if (Serial.available() >= 12) {
      //note that the tmElements_t Year member is an offset from 1970,
      //but the RTC wants the last two digits of the calendar year.
      //use the convenience macros from Time.h to do the conversions.
      int y = Serial.parseInt();
      if (y >= 100 && y < 1000)
        Serial.println("Error: Year must be two digits or four digits!");
      else {
        if (y >= 1000)
          tm.Year = CalendarYrToTm(y);
        else    //(y < 100)
          tm.Year = y2kYearToTm(y);
          tm.Month = Serial.parseInt();
          tm.Day = Serial.parseInt();
          tm.Hour = Serial.parseInt();
          tm.Minute = Serial.parseInt();
          tm.Second = Serial.parseInt();
          t = makeTime(tm);
    //use the time_t value to ensure correct weekday is set
        if(RTC.set(t) == 0) { // Success
          setTime(t);
          Serial.println("RTC set");
          //printDateTime(t);
          //Serial.println();
        }else
          Serial.println("RTC set failed!");
          //dump any extraneous input
          while (Serial.available() > 0) Serial.read();
      }
  }
}

#endif

void sendDataSerial() {
  //send to ESP8266 unit via UART
  //data sended:
  //I tempIN 
  //O tempOUT
  //1-x tempRad1-x
  //R relay status

  //data sended:
  //#0;25.31#1;25.19#2;5.19#I;25.10#O;50.5#R;1$3600177622*

  if (firstMeasComplete==false) return;

  Serial.print("DATA:");
  //digitalWrite(LEDPIN,HIGH);
  crc = ~0L;
  for (byte i=0;i<sensorsUT.getDeviceCount(); i++) {
    send(START_BLOCK);
    send(i);
    send(DELIMITER);
    send(tempUT[i]);
  }
  send(START_BLOCK);
  send('I');
  send(DELIMITER);
  send(tempIN);

  send(START_BLOCK);
  send('O');
  send(DELIMITER);
  send(tempOUT);

  send(START_BLOCK);
  send('R');
  send(DELIMITER);
  if (relay==LOW)
    send('1');
  else
    send('0');

  send(END_BLOCK);

  //mySerial.print(crc);
  send(END_TRANSMITION);
  mySerial.flush();
 
  Serial.println();
}

void send(char s) {
  send(s, ' ');
}


void send(char s, char type) {
  if (type=='X') {
#ifdef serial
    Serial.print(s, HEX);
#endif
    mySerial.print(s, HEX);
  }
  else {
#ifdef serial
    Serial.print(s);
#endif
    mySerial.print(s);
  }
  crc_string(byte(s));
}

void send(byte s) {
  send(s, ' ');
}

void send(byte s, char type) {
  if (type=='X') {
#ifdef serial
    Serial.print(s, HEX);
#endif
    mySerial.print(s, HEX);
  }
  else {
#ifdef serial
    Serial.print(s);
#endif
    mySerial.print(s);
  }
  crc_string(s);
}

void crc_string(byte s)
{
  crc = crc_update(crc, s);
  crc = ~crc;
}

unsigned long crc_update(unsigned long crc, byte data)
{
    byte tbl_idx;
    tbl_idx = crc ^ (data >> (0 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    tbl_idx = crc ^ (data >> (1 * 4));
    crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
    return crc;
}


void send(unsigned long s) {
#ifdef serial
  Serial.print(s);
#endif
  mySerial.print(s);
}

void send(unsigned int s) {
#ifdef serial
  Serial.print(s);
#endif
  mySerial.print(s);
}

void send(float s) {
  char tBuffer[8];
  dtostrf(s,0,2,tBuffer);
  for (byte i=0; i<8; i++) {
    if (tBuffer[i]==0) break;
    send(tBuffer[i]);
  }
}

void keyBoard() {
#ifdef keypad
  //char customKey = customKeypad.getKey();
  if (key!=' '){
    Serial.println(key);
    if (displayVar == 3) {
      if (key=='*') {
        displayVarSub=31;
        //Serial.println(displayVarSub);
      } else if (key=='D') {
        displayVarSub=32;
        //Serial.println(displayVarSub);
      }
      return;
    }

    if (key=='1') {
      lcd.clear();
      displayVar = 1;
    } else if (key=='2') {
      lcd.clear();
      displayVar = 2;
    } else if (key=='3') {
      lcd.clear();
      displayVar = 3;
    }
    /*
    Keyboard layout
    -----------
    | 1 2 3 A |
    | 4 5 6 B |
    | 7 8 9 C |
    | * 0 # D |
    -----------
    1 - total energy
    2 - TempDiffON
    3 - TempDiffOFF
    A - BACKLIGHT ON/OFF
    4 - Energy koef
    5 - Max IN OUT temp
    6 - Max bojler
    B - 
    7 - Max power today
    8 - Control sensor
    9 - total time
    C - DISPLAY CLEAR
    * - Save total energy to EEPROM
    0 -
    # - Select control sensor
    D - manual/auto
    */
    key = ' ';
  }
#endif
}

#ifdef keypad
uint8_t read8() {
  Wire.beginTransmission(address);
  Wire.requestFrom(address, 1);
  data = Wire.read();
  error = Wire.endTransmission();
  //Serial.println(error);
  return data;
}
/*
uint8_t read(uint8_t pin) {
  read8();
  return (data & (1<<pin)) > 0;
}
*/
void write8(uint8_t value) {
  Wire.beginTransmission(address);
  data = value;
  Wire.write(data);
  error = Wire.endTransmission();
}
/*
void write(uint8_t pin, uint8_t value) {
  read8();
  if (value == LOW) {
    data &= ~(1<<pin);
  }else{
    data |= (1<<pin);
  }
  write8(data); 
}
*/
void keyPressed() {
  byte row=0;
  byte col=255;
  byte b=127;
  if (millis() - lastKeyPressed > repeatAfterMs) {
    keyOld = 0;
  }
  //write8(1);
  //Serial.println(read8());
  
  for (byte i=0; i<4; i++) {
    row=i;
    b=~(255&(1<<i+4));
    //Serial.println(b);
    write8(b);
    //Serial.println(read8());
    col = colTest(read8(), b);
    //Serial.println(col);
    if (col<255) {
      key = hexaKeys[row][col];
      //Serial.print(key);
      if (key!=keyOld) {
        lastKeyPressed = millis();
        keyOld=hexaKeys[row][col];
        /*Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.print("=");
        Serial.println((char)key);
        */
      } else {
        key = ' ';
      }
      break;
    }
  }
}

byte colTest(byte key, byte b) {
  if (key==b-8) return 0;
  else if (key==b-4) return 1;
  else if (key==b-2) return 2;
  else if (key==b-1) return 3;
  else return 255;
}
#endif

void display() {
  if (displayVar==1) {
    displayTemp();
  } else if (displayVar==2) {
    displayInfo();
  } else if (displayVar==3) {
    setTempON();
  } else if (displayVar==31) {
    setTempON();
  } else if (displayVar==32) {
    setTempON();
  }
}

void displayInfo() {
  lcd.setCursor(0,0);
  lcd.print(versionSWString);
  lcd.print(versionSW);

  lcd.setCursor(0,1);
  lcd.print("Temp ON:");
  lcd.print(storage.tempON);

  lcd.setCursor(0,2);
  lcd.print("Temp OFF diff:");
  lcd.print(storage.tempOFFDiff);
  
  lcd.setCursor(0,3);
  lcd.print("Temp alarm:");
  lcd.print(storage.tempAlarm);
}

void setTempON() {
  lcd.setCursor(0,3);
  lcd.print("Temp ON:");
  if (displayVarSub==31) {
    storage.tempON++;
  }
  if (displayVarSub==32) {
    storage.tempON--;
  }
  displayVarSub=0;
  lcd.print(storage.tempON);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
