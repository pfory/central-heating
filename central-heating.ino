/*
--------------------------------------------------------------------------------------------------------------------------

               CENTRAL HEATING - control system for central heating

Petr Fory pfory@seznam.cz

Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating

Version history:
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
A2              - 
A3              - free
A4              - I2C display SDA 0x20, I2C Central heating unit 0x02
A5              - I2C display SCL 0x20, I2C Central heating unit 0x02
D0              - Rx
D1              - Tx
D2              - DALLAS
D3              - 
D4              - DALLAS
D5              - DALLAS
D6              - 
D7              - 
D8              - LED
D9              - BUZZER
D10             - free
D11             - free
D12             - free
D13             - free
--------------------------------------------------------------------------------------------------------------------------


//Rad1 - 
//Rad2 - 
//Rad3
//Rad4 - BedRoomNew
//Rad5 - LivingRoom OUT
//Rad6 - LivingRoom IN
//Rad7 -
//Rad8 - BedRoomNew
*/

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
#define BUZZERPIN                           9

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

const unsigned long   measDelay     = 5000; //in ms
unsigned long         lastMeas      = measDelay;
const unsigned long   measTime      = 750; //in ms
const unsigned long   sendDelay     = 20000; //in ms
unsigned long         lastSend      = sendDelay;
float                 tempOUT       = 0.f;
float                 tempIN        = 0.f;
float                 tempUT[10];
bool                  relay         = HIGH;

unsigned int const SERIAL_SPEED     = 9600;  //kvuli BT modulu jinak muze byt vice

#define CONFIG_START 0
#define CONFIG_VERSION "v07"

struct StoreStruct {
  // This is for mere detection if they are your settings
  char            version[4];
  // The variables of your settings
  unsigned int    moduleId;  // module id
  bool            relay;     // relay state
  float           tempON;
  float           tempOFFDiff;
  float           tempAlarm;
} storage = {
  CONFIG_VERSION,
  // The default module 0
  0,
  0, // off
  60,
  5,
  85
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

#define keypad
#ifdef keypad
int address                               = 0x27;
uint8_t data;
int error;
uint8_t pin                               = 0;
char key                                  = ' ';
char keyOld                               = ' ';
unsigned int repeatAfterMs                = 300;
unsigned int repeatCharSec                = 10;
unsigned long lastKeyPressed              = 0;

//define the symbols on the buttons of the keypads
const byte ROWS                           = 4; //four rows
const byte COLS                           = 4; //four columns
char hexaKeys[ROWS][COLS]                 = {
                                            {'*','0','#','D'},
                                            {'7','8','9','C'},
                                            {'4','5','6','B'},
                                            {'1','2','3','A'}
};
#endif



//SW name & version
float const   versionSW                   = 0.4;
char  const   versionSWString[]           = "Central heat v"; 

const byte STATUS_AFTER_BOOT  = 9;

/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void)
{
  Serial.begin(SERIAL_SPEED);
  Serial.print(versionSWString);
  Serial.println(versionSW);
  beep.Delay(100,40,1,255);
  
  lcd.begin(LCDCOLS,LCDROWS);               // initialize the lcd 
  lcd.setBacklight(255);
  lcd.clear();
  lcd.print(versionSWString);
  lcd.print(versionSW);

  loadConfig();
 
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,!relay);
  pinMode(BUZZERPIN, OUTPUT);
  
  while (true) {
    sensorsOUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
    sensorsOUT.setResolution(12);
    sensorsOUT.setWaitForConversion(false);
    sensorsIN.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
    sensorsIN.setResolution(12);
    sensorsIN.setWaitForConversion(false);
    sensorsUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
    sensorsUT.setResolution(12);
    sensorsUT.setWaitForConversion(false);

    if (sensorsIN.getDeviceCount()==0 || sensorsOUT.getDeviceCount()==0) {
      beep.Delay(100,40,2,255);
    } else {
      break;
    }
    delay(60000);
  }

  
  Serial.println();
  Serial.print("Sensor(s) ");
  Serial.print(sensorsIN.getDeviceCount());
  Serial.print(" on bus IN - pin ");
  Serial.println(ONE_WIRE_BUS_IN);

  lcd.setCursor(0,1);
  lcd.print("Sen.");
  lcd.print(sensorsIN.getDeviceCount());
  lcd.print(" bus IN");
   
  lcd.setCursor(0,2);
  lcd.print("Sen.");
  lcd.print(sensorsOUT.getDeviceCount());
  lcd.print(" bus OUT");
 
  lcd.setCursor(0,3);
  lcd.print("Sen.");
  lcd.print(sensorsUT.getDeviceCount());
  lcd.print(" bus UT");

  Serial.print("Sensor(s) ");
  Serial.print(sensorsOUT.getDeviceCount());
  Serial.print(" on bus OUT - pin ");
  Serial.println(ONE_WIRE_BUS_OUT);
  Serial.print("Sensor(s) ");
  Serial.print(sensorsUT.getDeviceCount());
  Serial.print(" on bus UT - pin ");
  Serial.println(ONE_WIRE_BUS_UT);
  Serial.print("Send interval ");
  Serial.print(sendDelay);
  Serial.println(" sec");
  Serial.print("Temp ON ");
  Serial.println(storage.tempON);
  Serial.print("Temp OFF diff ");
  Serial.println(storage.tempOFFDiff);
  Serial.print("Temp alarm ");
  Serial.println(storage.tempAlarm);

  
  delay(3000);
  lcd.clear();
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

  delay(3000);
  lcd.clear();

  wdt_enable(WDTO_8S);
  
  lastSend=0;
  lastMeas=0;
}

//bool first=true;
/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
/*  if (first) {
    beep.noDelay(100,40,4,255);
    first=false;
  }
*/  
  wdt_reset();
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    startMeas();    
    delay(measTime);
    getTemp();
    printTemp();
    Wire.beginTransmission(8);
    Wire.write((byte)tempOUT);
    Wire.endTransmission();
  
    if (tempOUT <= storage.tempON - storage.tempOFFDiff) {
      //Serial.println("Relay OFF");
      relay = HIGH;
    }
    if ((tempOUT >= storage.tempON) || (tempIN >= storage.tempON)) {
      //Serial.println("Relay ON");
      relay = LOW;
    }

    digitalWrite(RELAYPIN,relay);
    digitalWrite(LEDPIN,!relay);

    displayTemp();
    
    if (millis() - lastSend >= sendDelay) {
      lastSend = millis();

      //send to solar unit via I2C
      //data sended:
      //I tempIN 
      //O tempOUT
      //1-x tempRad1-x
      //R relay status
      Wire.beginTransmission(9);
      Wire.write("I");
      Wire.write((int)tempIN);
      Wire.write("O");
      Wire.write((int)tempOUT);
      Wire.write("R");
      Wire.write(relay);
      for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
        Wire.write(i+1);
        Wire.write((int)tempUT[i]);
      }
      Wire.endTransmission();
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
  }
  
  beep.loop();
  keyPressed();

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
  if (sensorsIN.getCheckForConversion()==true) {
    tempIN = sensorsIN.getTempCByIndex(0);
  }
  if (sensorsOUT.getCheckForConversion()==true) {
    tempOUT = sensorsOUT.getTempCByIndex(0);
  }
  if (sensorsUT.getCheckForConversion()==true) {
    for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
      tempUTRaw[i]=sensorsUT.getTempCByIndex(i);
    }
    
    //remaping temp
    tempUT[0]=tempUTRaw[5];
    tempUT[1]=tempUTRaw[4];
    tempUT[2]=tempUTRaw[3];
    tempUT[3]=tempUTRaw[7];
    tempUT[4]=tempUTRaw[0];
    tempUT[5]=tempUTRaw[1];
    tempUT[6]=tempUTRaw[2];
    tempUT[7]=tempUTRaw[6];
/*  tempUT[8]=tempUTRaw[];
    tempUT[9]=tempUTRaw[];
    tempUT[10]=tempUTRaw[];
    tempUT[11]=tempUTRaw[];*/
  }
}

void printTemp() {
  Serial.print("Temp IN: ");
  Serial.print(tempIN); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  Serial.println();
  Serial.print("Temp OUT: ");
  Serial.print(tempOUT); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  Serial.println();
  for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    Serial.print("Temp UT[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.print(tempUT[i]); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
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
0|xxx/xxx     15:30:45
1|1:xxx/xxx 2:xxx/xxx 
2|3:xxx/xxx 4:xxx/xxx 
3|5:xxx/xxx 6:xxx/xxx 
  --------------------
  01234567890123456789
*/

void displayTemp() {
  lcd.setCursor(0, 0); //col,row
  lcd.print("       ");
  lcd.setCursor(0, 0); //col,row
  addSpaces((int)tempIN);
  lcd.print((int)tempIN);
  lcd.setCursor(3, 0); //col,row
  lcd.print("/");
  //addSpaces((int)tempOUT);
  lcd.print((int)tempOUT);
  lcd.setCursor(12, 0); //col,row
  lcd.print("15:30:45");

  byte radka=1;
  byte sensor=0;
  byte radiator=1;
  for (byte i=0; i<sensorsUT.getDeviceCount(); i=i+4) {
    lcd.setCursor(0, radka);
    lcd.print(radiator++);
    lcd.print(":       ");
    lcd.setCursor(2, radka);
    addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
    lcd.setCursor(5, radka);
    lcd.print("/");
    //addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
    lcd.setCursor(10, radka);
    lcd.print(radiator++);
    lcd.print(":       ");
    lcd.setCursor(12, radka);
    addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
    lcd.setCursor(15, radka++);
    lcd.print("/");
    //addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
  }

  lcd.setCursor(8, 0);
  if (relay==HIGH) {
    lcd.print("    ");
  }else{
    lcd.print("CER");
  }
}

void addSpaces(int cislo) {
  if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  if (cislo<=0 && cislo>-10) lcd.print(" ");
}


#ifdef keypad
uint8_t read8() {
  Wire.beginTransmission(address);
  Wire.requestFrom(address, 1);
  data = Wire.read();
  error = Wire.endTransmission();
  return data;
}

uint8_t read(uint8_t pin) {
  read8();
  return (data & (1<<pin)) > 0;
}

void write8(uint8_t value) {
  Wire.beginTransmission(address);
  data = value;
  Wire.write(data);
  error = Wire.endTransmission();
}

void write(uint8_t pin, uint8_t value) {
  read8();
  if (value == LOW) {
    data &= ~(1<<pin);
  }else{
    data |= (1<<pin);
  }
  write8(data); 
}

void keyPressed() {
  byte row=0;
  byte col=255;
  byte b=127;
  if (millis() - lastKeyPressed > repeatAfterMs) {
    keyOld = 0;
  }
  for (byte i=0; i<4; i++) {
    row=i;
    b=~(255&(1<<i+4));
    //Serial.println(b);
    write8(b);
    col = colTest(read8(), b);
    if (col<255) {
      key = hexaKeys[row][col];
      if (key!=keyOld) {
        lastKeyPressed = millis();
        keyOld=hexaKeys[row][col];
        Serial.print(row);
        Serial.print(",");
        Serial.print(col);
        Serial.print("=");
        Serial.println((char)key);
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