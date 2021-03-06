/*
--------------------------------------------------------------------------------------------------------------------------
CENTRAL HEATING - control system for central heating
Petr Fory pfory@seznam.cz
GIT - https://github.com/pfory/central-heating
*/

#include "Configuration.h"

#define watchdog //enable this only on board with optiboot bootloader
#ifdef watchdog
#include <avr/wdt.h>
#endif


#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWireOUT(ONE_WIRE_BUS_OUT);
OneWire oneWireIN(ONE_WIRE_BUS_IN);
OneWire oneWireUT(ONE_WIRE_BUS_UT);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsOUT(&oneWireOUT);
DallasTemperature sensorsIN(&oneWireIN);
DallasTemperature sensorsUT(&oneWireUT);

DeviceAddress inThermometer, outThermometer;
DeviceAddress utT[15];

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
bool                  tempRefresh              = false;
unsigned long         hbDelay                  = 500;
byte                  hb                       = 0;
unsigned long         lastHB                   = hbDelay * -1;


                    
#define beep
// #ifdef beep
// #include "beep.h"
// Beep peep(BUZZERPIN);
// #endif

#define DS1307
#ifdef DS1307
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeLib.h>
#define time
#endif


#ifdef time
#include <Time.h>
//#include <Streaming.h>        
#include <Time.h>             
//bool parse=false;
//bool config=false;
tmElements_t    tm;
//bool isTime = true;
#endif




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
  45,
  5,
  95
};

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCDADDRESS,LCDCOLS,LCDROWS);  // set the LCD
bool backLight = true;

#include <SoftwareSerial.h>
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


#include <Keypad_I2C.h>
#include <Keypad.h>          // GDY120705
#include <Wire.h>


const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS]                 = {
                                            {'1','2','3','A'},
                                            {'4','5','6','B'},
                                            {'7','8','9','C'},
                                            {'*','0','#','D'}
};
byte rowPins[ROWS] = {7,6,5,4}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {3,2,1,0}; //connect to the column pinouts of the keypad

//Keypad_I2C keypad = Keypad_I2C( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR); 
// #endif

byte displayVar=1;
char displayVarSub=' ';


const byte STATUS_AFTER_BOOT  = 9;

/////////////////////////////////////////////   S  E  T  U  P   ////////////////////////////////////
void setup(void) {
#ifdef serial
  Serial.begin(SERIAL_SPEED);
  Serial.print(F(SW_NAME));
  Serial.print(F(" "));
  Serial.println(F(VERSION));
#endif

  /*storage.tempON = 45;
  storage.tempOFFDiff = 5;
  saveConfig();
*/
#ifdef beep
  //peep.Delay(100,40,1,255);
  tone(BUZZERPIN, 5000, 5);
#endif  
  lcd.init();               // initialize the lcd 
  lcd.backlight();
  lcd.home();
  lcd.print(SW_NAME);
  PRINT_SPACE
  lcd.print(VERSION);
  
  mySerial.begin(mySERIAL_SPEED);
  
  keypad.begin();
  
#ifdef DS1307
  if (RTC.read(tm)) {
    Serial.print(F("RTC OK, Time = "));
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(F(", Date (D/M/Y) = "));
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    //Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(tm.Year);
    Serial.println();
  } else {
    if (RTC.chipPresent()) {
      Serial.println(F("The DS1307 is stopped.  Please run the SetTime"));
      Serial.println(F("example to initialize the time and begin running."));
      Serial.println();
    } else {
      Serial.println(F("DS1307 read error!  Please check the circuitry."));
      Serial.println();
    }
  }
#endif

#ifdef time
  // Setup time library  
  Serial.print(F("RTC Sync"));
  setSyncProvider(RTC.get);          // the function to get the time from the RTC
  if(timeStatus() == timeSet) {
    Serial.println(F(" OK!"));
  } else {
    Serial.println(F(" FAIL!"));
  }
#endif
  loadConfig();
  Serial.print(F("Temp ON "));
  Serial.println(storage.tempON);
  Serial.print(F("Temp OFF diff "));
  Serial.println(storage.tempOFFDiff);
  Serial.print(F("Temp alarm "));
  Serial.println(storage.tempAlarm);
  delay(5000);
  lcd.clear();
 
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,!relay);
  while (true) {
    sensorsOUT.begin(); 
    sensorsIN.begin(); 
    sensorsUT.begin(); 

    if (sensorsIN.getDeviceCount()==0 || sensorsOUT.getDeviceCount()==0) {
#ifdef beep
      //peep.Delay(100,40,1,255);
#endif
      Serial.println(F("NO temperature sensor(s) DS18B20 found!!!!!!!!!"));
      lcd.setCursor(0, 1);
      lcd.print(F("!NO temp.sensor(s)!!"));
      lcd.setCursor(0, 2);
      lcd.print(F("!!!DS18B20 found!!!!"));
      lcd.setCursor(0, 3);
      lcd.print(F("!!!!!Check wire!!!!!"));
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
  Serial.print(F("Sensor(s) "));
  Serial.print(sensorsIN.getDeviceCount());
  Serial.print(F(" on bus IN - pin "));
  Serial.println(ONE_WIRE_BUS_IN);
  Serial.print(F("Device Address: "));
  sensorsIN.getAddress(inThermometer, 0); 
  printAddress(inThermometer);
  Serial.println();
  
  
  /*
  uint8_t adresse[8];
char adresse_formatee_hexa[24];

sprintf(adresse_formatee_hexa,
        "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
        adresse[0],
        adresse[1],
        adresse[2],
        adresse[3],
        adresse[4],
        adresse[5],
        adresse[6],
        adresse[7]);
*/
  lcd.setCursor(0,1);
  lcd.print(F("Sen."));
  lcd.print(sensorsIN.getDeviceCount());
  lcd.print(F(" bus IN"));

  Serial.print(F("Sensor(s) "));
  Serial.print(sensorsOUT.getDeviceCount());
  Serial.print(F(" on bus OUT - pin "));
  Serial.println(ONE_WIRE_BUS_OUT);
  Serial.print(F("Device Address: "));
  sensorsOUT.getAddress(outThermometer, 0); 
  printAddress(outThermometer);
  Serial.println();
   
  lcd.setCursor(0,2);
  lcd.print(F("Sen."));
  lcd.print(sensorsOUT.getDeviceCount());
  lcd.print(F(" bus OUT"));
 
  Serial.print(F("Sensor(s) "));
  Serial.print(sensorsUT.getDeviceCount());
  Serial.print(F(" on bus UT - pin "));
  Serial.println(ONE_WIRE_BUS_UT);
  Serial.println(F("Device Address: "));
  for (byte i = 0; i<sensorsUT.getDeviceCount(); i++) {
    sensorsUT.getAddress(utT[i], i); 
    printAddress(utT[i]);
    Serial.println();
  }
  
  lcd.setCursor(0,3);
  lcd.print(F("Sen."));
  lcd.print(sensorsUT.getDeviceCount());
  lcd.print(F(" bus UT"));

  
  
  Serial.print(F("Send interval "));
  Serial.print(sendDelay);
  Serial.println(F(" sec"));
  
  delay(1000);
  lcd.clear();
  
  displayInfo();

  delay(1000);
  lcd.clear();

  lastSend=0;
  //lastMeas=-measDelay;
  Serial.println(F("Setup end."));
#ifdef watchdog
  wdt_enable(WDTO_8S);
#endif

}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
#ifdef watchdog
  wdt_reset();
#endif  
#ifdef time
  //setTime();
#endif  
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    Serial.println(millis());
    startMeas(); 
    startConversion=true;
    startConversionMillis = millis();
  }
  if (startConversion && (millis() - startConversionMillis >= measTime)) {
    startConversion=false;
    getTemp();
    //printTemp();
    tempRefresh = true;
    //poslani teploty do LED displeje
    Wire.beginTransmission(8);
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
      Serial.println(F("Relay ON"));
    } else {
      Serial.println(F("Relay OFF"));
    }

    digitalWrite(RELAYPIN,!relay);
    digitalWrite(LEDPIN,relay);
  }
  if (millis() - lastSend >= sendDelay) {
    sendDataSerial();
    lastSend = millis();
  }

  if (tempOUT >= storage.tempAlarm) {
#ifdef beep    
    //peep.noDelay(100,40,3,255);
#endif
  }
  /*    if ((tempOUT || tempIN) >= storage.tempAlarm && (tempOUT || tempIN) < storage.tempAlarm+5) {
    peep.noDelay(100,40,3,255);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+5 && (tempOUT || tempIN) < storage.tempAlarm+10) {
    peep.noDelay(100,40,5,255);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+10) {
    peep.noDelay(100,40,9,255);
  } */

  display();
#ifdef beep  
  //peep.loop();
#endif  
  keyBoard();
  
#ifdef time
  testPumpProtect();
#endif
}

/////////////////////////////////////////////   F  U  N  C   ///////////////////////////////////////
void startMeas() {
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print(F("Requesting temperatures..."));
  sensorsIN.requestTemperatures(); // Send the command to get temperatures
  sensorsOUT.requestTemperatures(); // Send the command to get temperatures
  sensorsUT.requestTemperatures(); // Send the command to get temperatures
  Serial.println(F("DONE"));
}

void getTemp() {
  //DeviceAddress da;
  tempIN = sensorsIN.getTempCByIndex(0);
  tempOUT = sensorsOUT.getTempCByIndex(0);

  tempUT[0]=sensorsUT.getTempCByIndex(5);   //obyvak vstup
  tempUT[1]=sensorsUT.getTempCByIndex(4);   //obyvak vystup
  tempUT[2]=sensorsUT.getTempCByIndex(3);   //loznice nova vstup
  tempUT[3]=sensorsUT.getTempCByIndex(11);  //loznice nova vystup
  tempUT[4]=sensorsUT.getTempCByIndex(2);   //loznice stara vstup
  tempUT[5]=sensorsUT.getTempCByIndex(9);   //loznice stara vystup 
  tempUT[6]=sensorsUT.getTempCByIndex(1);   //dilna vstup
  tempUT[7]=sensorsUT.getTempCByIndex(0);   //dilna vystup
  tempUT[8]=sensorsUT.getTempCByIndex(8);   //bojler vstup
  tempUT[9]=sensorsUT.getTempCByIndex(6);   //bojler vystup
  tempUT[10]=sensorsUT.getTempCByIndex(7);  //chodba vstup
  tempUT[11]=sensorsUT.getTempCByIndex(10); //chodba vystup
  

  /*
  0 - 28FF60AA9015018E
  1 - 28FFF438901501C0
  2 - 28FF0C90901501ED
  3 - 28FF62DF90150124
  4 - 28FF0AE2901501DE
  5 - 28FF79E2901501FD
  6 - 28FFA568741603F0
  7 - 28FFE5557416044F
  8 - 28FF6D2F7316052D
  9 - 28FFE3D79015013E
  10 - 28FFBB2F00160185
  11 - 28FF07E090150135


  obyvak
  I - 28FF79E2901501FD
  O - 28FF0AE2901501DE

  loznice nova
  I - 28FF62DF90150124
  O - 28FF07E090150135

  loznice stara
  I - 28FF0C90901501ED
  O - 28FFE3D79015013E

  dilna
  I - 28FFF438901501C0
  O - 28FF60AA9015018E

  bojler
  I - 28FF6D2F7316052D
  O - 28FFA568741603F0

  chodba
  I - 28FFE5557416044F
  O - 28FFBB2F00160185
*/

  // for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {

      // tempUT[i]=sensorsUT.getTempCByIndex(i);
    // /*sensorsUT.getAddress(da, i); 
    // printAddress(da);
    // Serial.println();
    // if (da[7]=="142") {
      // tempUT[4]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="192") {
      // tempUT[5]=sensorsUT.getTempCByIndex(i);
    // } else if(da[7]=="237") {
      // tempUT[6]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="36") {
      // tempUT[2]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="222") {
      // tempUT[1]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="30") {
      // tempUT[0]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="253") {
      // tempUT[7]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="62") {
      // tempUT[8]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="133") {
      // tempUT[3]=sensorsUT.getTempCByIndex(i);
    // } else if (da[7]=="53") {
      // tempUT[9]=sensorsUT.getTempCByIndex(i);
    // }*/
  // }

  firstMeasComplete=true;
}

void printTemp() {
  Serial.print(F("Temp IN: "));
  Serial.print(tempIN); 
  Serial.println();
  Serial.print(F("Temp OUT: "));
  Serial.print(tempOUT); 
  Serial.println();
  DeviceAddress da;
  
  Serial.print(tempUT[0]);
  Serial.print(" obyvak vstup - ");
  sensorsUT.getAddress(da, 4); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[1]);
  Serial.print(" obyvak vystup - ");
  sensorsUT.getAddress(da, 5); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[2]);
  Serial.print(" loznice nova vstup - ");
  sensorsUT.getAddress(da, 3); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[3]);
  Serial.print(" loznice nova vystup - ");
  sensorsUT.getAddress(da, 11); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[4]);
  Serial.print(" loznice stara vstup - ");
  sensorsUT.getAddress(da, 2); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[5]);
  Serial.print(" loznice stara vystup - ");
  sensorsUT.getAddress(da, 9); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[6]);
  Serial.print(" dilna vstup - ");
  sensorsUT.getAddress(da, 0); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[7]);
  Serial.print(" dilna vystup - ");
  sensorsUT.getAddress(da, 1); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[8]);
  Serial.print(" bojler vstup - ");
  sensorsUT.getAddress(da, 8); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[9]);
  Serial.print(" bojler vystup - ");
  sensorsUT.getAddress(da, 6); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[10]);
  Serial.print(" hala vstup - ");
  sensorsUT.getAddress(da, 7); 
  printAddress(da);
  Serial.println();
  Serial.print(tempUT[11]);
  Serial.print(" hala vystup - ");
  sensorsUT.getAddress(da, 10); 
  printAddress(da);
  Serial.println();

 /* for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    Serial.print(F("Temp UT["));
    Serial.print(i);
    Serial.print(F("]: "));
    Serial.print(tempUT[i]); 
    Serial.print(" - ");
    sensorsUT.getAddress(da, i); 
    printAddress(da);
    Serial.println();
  }*/
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
  lcd.setCursor(0,3);
  lcd.print(F("Saved to EEPROM."));
  delay(2000);
  displayVar=1;
  lcd.clear();
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
  //displayTime();
  lcd.setCursor(15, 0); //col,row
  lcd.print((int)storage.tempON);
  lcd.print("/");
  lcd.print((int)(storage.tempON-storage.tempOFFDiff));
#else
  lcd.setCursor(19, 0); //col,row
  if (millis() - lastHB >=hbDelay) {
    lastHB = millis();
    if (hb==1) {
      hb=0;
      lcd.print("*");
    } else {
      hb=1;
      lcd.print(" ");
    }
  }
#endif
  if (!tempRefresh) {
    return;
  }
  tempRefresh=false;
  lcd.setCursor(0, 0); //col,row
  lcd.print(F("       "));
  lcd.setCursor(0, 0); //col,row
  if (tempIN==TEMP_ERR) {
    displayTempErr();
  }else {
    addSpacesBefore((int)tempIN);
    lcd.print((int)tempIN);
  }
  lcd.setCursor(2, 0); //col,row
  lcd.print(F("/"));
  //addSpaces((int)tempOUT);
  if (tempOUT==TEMP_ERR) {
    displayTempErr();
  }else {
    lcd.print((int)tempOUT);
  }
  /*
  byte radka=1;
  byte sensor=0;
  byte radiator=1;
  for (byte i=0; i<sensorsUT.getDeviceCount(); i=i+6) {
    displayRadTemp(0, radka, sensor);
    sensor+=2;
    displayRadTemp(7, radka, sensor);
    sensor+=2;
    displayRadTemp(14, radka, sensor);
    sensor+=2;
    radka++;
  }
  */
  lcd.setCursor(8, 0);
  if (relay==HIGH) {
    lcd.print("    ");
  }else{
    lcd.print("CER");
  }
}

void displayRadTemp(byte sl, byte rad, byte s) {
    lcd.setCursor(sl, rad);
    if (tempUT[s]==TEMP_ERR) {
      displayTempErr();
    }else {
      addSpacesBefore((int)tempUT[s]);
      lcd.print((int)tempUT[s]);
    }
    lcd.setCursor(sl+2, rad);
    lcd.print(F("/"));
    s++;
    if (tempUT[s]==TEMP_ERR) {
      displayTempErr();
    }else {
      lcd.print((int)tempUT[s]);    
      addSpacesAfter((int)tempUT[s]);
    }
}

void displayTempErr() {
  lcd.print("ER");
}

void addSpacesBefore(int cislo) {
  //if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  //if (cislo<=0 && cislo>-10) lcd.print(" ");
}

void addSpacesAfter(int cislo) {
  //if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  //if (cislo<=0 && cislo>-10) lcd.print(" ");
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

#ifdef time
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
#endif

//zabranuje zatuhnuti cerpadla v lete
void testPumpProtect() {
  //if (storage.lastPumpRun
}

// void setTime() {
  // static time_t tLast;
  // time_t t;
  
  // tmElements_t tm;
  // //check for input to set the RTC, minimum length is 12, i.e. yy,m,d,h,m,s
  // if (Serial.available() >= 12) {
      // //note that the tmElements_t Year member is an offset from 1970,
      // //but the RTC wants the last two digits of the calendar year.
      // //use the convenience macros from Time.h to do the conversions.
      // int y = Serial.parseInt();
      // if (y >= 100 && y < 1000)
        // Serial.println(F("Error: Year must be two digits or four digits!"));
      // else {
        // if (y >= 1000)
          // tm.Year = CalendarYrToTm(y);
        // else    //(y < 100)
          // tm.Year = y2kYearToTm(y);
          // tm.Month = Serial.parseInt();
          // tm.Day = Serial.parseInt();
          // tm.Hour = Serial.parseInt();
          // tm.Minute = Serial.parseInt();
          // tm.Second = Serial.parseInt();
          // t = makeTime(tm);
    // //use the time_t value to ensure correct weekday is set
        // if(RTC.set(t) == 0) { // Success
          // setTime(t);
          // Serial.println(F("RTC set"));
          // //printDateTime(t);
          // //Serial.println();
        // }else
          // Serial.println(F("RTC set failed!"));
          // //dump any extraneous input
          // while (Serial.available() > 0) Serial.read();
      // }
  // }
// }

#endif

void sendDataSerial() {
  //send to ESP8266 unit via UART
  //data sended:
  //I tempIN 
  //O tempOUT
  //1-x tempRad1-x
  //R relay status

  //data sended:
  //#0;59.81#1;32.88#2;48.44#3;30.44#4;8.94#5;8.69#6;17.06#7;14.06#8;36.44#9;36.44#A;11.06#B;11.13#I;55.13#O;62.44#R;0$*

  if (firstMeasComplete==false) return;

  Serial.print("DATA:");
  crc = ~0L;
  for (byte i=0;i<sensorsUT.getDeviceCount(); i++) {
    send(START_BLOCK);
    send(i, 'X');
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
  char key = keypad.getKey();
  if (key!=NO_KEY) {
    Serial.println(key);
    lcd.clear();
    if (key=='1') {
      displayVar = 1;
      lcd.clear();
    } else if (key=='2') {
      displayVar = 2;
      lcd.clear();
    } else if (key=='3') {
      displayVar = 3;
      lcd.clear();
    } else if (key=='4') {
      displayVar = 4;
      lcd.clear();
    } else if (key=='5') {
      displayVar = 5;
      lcd.clear();
    } else if (key=='6') {
      displayVar = 60;
      lcd.clear();
    }
    displayVarSub=key;
    
    /*
    Keyboard layout
    -----------
    | 1 2 3 A |
    | 4 5 6 B |
    | 7 8 9 C |
    | * 0 # D |
    -----------
    1 - normal - zobrazeni teplot
    2 - verze FW, teoplota spinaní, diference, teplota alarmu
    3 - nastavení teploty spínání
    A - 
    4 - nastavení teploty diference
    5 - nastavení teploty alarmu
    6 - nastavení hodin
    B - 
    7 - 
    8 - 
    9 - 
    C - 
    * - 
    0 -
    # - 
    D - 
    */
    key = ' ';
  }
}

void display() {
  if (displayVar==1) {
    displayTemp();
  } else if (displayVar==2) {
    displayInfo();
  } else if (displayVar==3) {
    setTempON();
  } else if (displayVar==4) {
    setTempDiff();
  } else if (displayVar==5) {
    setTempAlarm();
  } else if (displayVar>=60 && displayVar<=62) {
#ifdef time   
    setClock(displayVar);
#endif
  }
}

void displayInfo() {
  lcd.setCursor(0,0);
  lcd.print(F(SW_NAME));
  lcd.print(F(" "));
  lcd.print(F(VERSION));

  lcd.setCursor(0,1);
  lcd.print(F("Temp ON:"));
  lcd.print(storage.tempON);

  lcd.setCursor(0,2);
  lcd.print(F("Temp OFF diff:"));
  lcd.print(storage.tempOFFDiff);
  
  lcd.setCursor(0,3);
  lcd.print(F("Temp alarm:"));
  lcd.print(storage.tempAlarm);
}

void setTempON() {
  showHelpKey();
  lcd.print(F("Temp ON:"));
  if (displayVarSub=='*') {
    storage.tempON++;
  } else if (displayVarSub=='#') {
    storage.tempON--;
  } else if (displayVarSub=='D') {
    saveConfig();
  }
  lcd.print(storage.tempON);
  displayVarSub=' ';
}

void setTempDiff() {
  showHelpKey();
  lcd.print(F("Temp diff:"));
  if (displayVarSub=='*') {
    storage.tempOFFDiff++;
  } else if (displayVarSub=='#') {
    storage.tempOFFDiff--;
  } else if (displayVarSub=='D') {
    saveConfig();
  }
  lcd.print(storage.tempOFFDiff);
  displayVarSub=' ';
}

void setTempAlarm() {
  showHelpKey();
  lcd.print(F("Temp alarm:"));
  if (displayVarSub=='*') {
    storage.tempAlarm++;
  } else if (displayVarSub=='#') {
    storage.tempAlarm--;
  } else if (displayVarSub=='D') {
    saveConfig();
  }
  lcd.print(storage.tempAlarm);
  displayVarSub=' ';
}

void controlRange(uint8_t *pTime, uint8_t min, uint8_t max) {
   if (*pTime<min) {
     *pTime=min;
   }
   if (*pTime>max) {
     *pTime=max;
   }
  
}

#ifdef time
void setClock(byte typ) {
  if (typ==60) { //hour
    showHelpKey1();
    lcd.print(F("Hour:"));
    if (displayVarSub=='*') {
      tm.Hour++;
      controlRange(&tm.Hour, 0, 23);
    } else if (displayVarSub=='#') {
      tm.Hour--;
      controlRange(&tm.Hour, 0, 23);
    } else if (displayVarSub=='D') {
      displayVar=61;
    }
    displayVarSub=' ';
  } else if (typ==61) { //min
    showHelpKey1();
    lcd.print(F("Min:"));
    if (displayVarSub=='*') {
      tm.Minute++;
      controlRange(&tm.Minute, 0, 59);
    } else if (displayVarSub=='#') {
      tm.Minute--;
      controlRange(&tm.Minute, 0, 59);
    } else if (displayVarSub=='D') {
      displayVar=62;
    }
    displayVarSub=' ';
  } else if (typ==62) { //sec
    showHelpKey();
    lcd.print(F("Sec:"));
    if (displayVarSub=='*') {
      tm.Second++;
      controlRange(&tm.Second, 0, 59);
    } else if (displayVarSub=='#') {
      tm.Second--;
      controlRange(&tm.Second, 0, 59);
    } else if (displayVarSub=='D') {
      RTC.write(tm);
      displayVar=1;
    }
    lcd2digits(tm.Hour);
    lcd.write(':');
    lcd2digits(tm.Minute);
    lcd.write(':');
    lcd2digits(tm.Second);
    displayVarSub=' ';
  }
}
#endif

void showHelpKey() {
  lcd.setCursor(0,0);
  lcd.print(F("key * UP "));
  lcd.setCursor(0,1);
  lcd.print(F("key # DOWN "));
  lcd.setCursor(0,2);
  lcd.print(F("key D SAVE "));
  lcd.setCursor(0,3);
}

void showHelpKey1() {
  lcd.setCursor(0,0);
  lcd.print(F("key * UP "));
  lcd.setCursor(0,1);
  lcd.print(F("key # DOWN "));
  lcd.setCursor(0,2);
  lcd.print(F("key D NEXT "));
  lcd.setCursor(0,3);
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
