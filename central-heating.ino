#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//wiring
#define ONE_WIRE_BUS_IN                     10
#define ONE_WIRE_BUS_OUT                    11
#define ONE_WIRE_BUS_UT                     12

#define RELAYPIN                            5
#define LEDPIN                              6
#define BUZZERPIN                           7

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWireOUT(ONE_WIRE_BUS_OUT);
OneWire oneWireIN(ONE_WIRE_BUS_IN);
OneWire oneWireUT(ONE_WIRE_BUS_UT);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsOUT(&oneWireOUT);
DallasTemperature sensorsIN(&oneWireIN);
DallasTemperature sensorsUT(&oneWireUT);

const unsigned long   measDelay     = 1000; //in ms
unsigned long         lastMeas      = measDelay;
const unsigned long   dispDelay     = 5000; //in ms
unsigned long         lastDisp      = dispDelay;
float                 tempOUT       = 0;
float                 tempIN        = 0;
float                 tempUT[10];
float                 tempON        = 50;
float                 tempOFF       = tempON - 5;
float                 tempOVER      = 100;
bool                  tempINReady   = false;
bool                  tempOUTReady  = false;
bool                  tempUTReady   = false;

//HIGH - relay OFF, LOW - relay ON
bool relay                               = HIGH; 

unsigned int const SERIAL_SPEED=115200;


#include <LiquidCrystal_I2C.h>
#define LCDADDRESS   0x20
#define EN           2
#define RW           1
#define RS           0
#define D4           4
#define D5           5
#define D6           6
#define D7           7
#define BACKLIGHT    3
#define POL          POSITIVE
#define LCDROWS      2
#define LCDCOLS      16
LiquidCrystal_I2C lcd(LCDADDRESS,EN,RW,RS,D4,D5,D6,D7,BACKLIGHT,POL);  // set the LCD
//LiquidCrystal_I2C lcd(LCDADDRESS,2,16);  // set the LCD


//SW name & version
float const   versionSW                   = 0.1;
char  const   versionSWString[]           = "Central heat v"; 


void setup(void)
{
  Serial.begin(SERIAL_SPEED);
  Serial.print(versionSWString);
  Serial.println(versionSW);
  /*
  lcd.home();                   // go home
  lcd.print(versionSWString);  
  lcd.print(" ");
  lcd.print (versionSW);
  delay(1000);
  lcd.clear();
  */

  // Start up the library
  sensorsOUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  sensorsOUT.setResolution(12);
  sensorsOUT.setWaitForConversion(false);
  sensorsIN.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  sensorsIN.setResolution(12);
  sensorsIN.setWaitForConversion(false);
  sensorsUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  sensorsUT.setResolution(12);
  sensorsUT.setWaitForConversion(false);
  
  Serial.print("Sensor(s) ");
  Serial.print(sensorsIN.getDeviceCount());
  Serial.print(" on bus IN - pin ");
  Serial.println(ONE_WIRE_BUS_IN);
  Serial.print("Sensor(s) ");
  Serial.print(sensorsOUT.getDeviceCount());
  Serial.print(" on bus OUT - pin ");
  Serial.println(ONE_WIRE_BUS_OUT);
  Serial.print("Sensor(s) ");
  Serial.print(sensorsUT.getDeviceCount());
  Serial.print(" on bus UT - pin ");
  Serial.println(ONE_WIRE_BUS_UT);
  wdt_enable(WDTO_8S);
}


void loop(void)
{ 
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    startMeas();    
  } else {
    getTemp();
  }
  
  if (tempINReady && tempOUTReady && tempUTReady) {
    if (millis() - lastDisp >= dispDelay) {
      lastDisp = millis();
      displayTemp();
    }
  }

  if (tempOUT >= tempON) {
    relay = LOW;
    
  }
  if (tempOUT <= tempOFF) {
    relay = HIGH;
  }
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,relay);
  if (tempOUT || tempIN >= tempOVER) {
    digitalWrite(BUZZERPIN,HIGH);
  } else {
    digitalWrite(BUZZERPIN,LOW);
  }
}


void startMeas() {
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  tempINReady = tempOUTReady = tempUTReady = false;
  Serial.print("Requesting temperatures...");
  sensorsIN.requestTemperatures(); // Send the command to get temperatures
  sensorsOUT.requestTemperatures(); // Send the command to get temperatures
  sensorsUT.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
}

void getTemp() {
  if (sensorsIN.getCheckForConversion()==true) {
    tempIN = sensorsIN.getTempCByIndex(0);
    tempINReady = true;
  }
  if (sensorsOUT.getCheckForConversion()==true) {
    tempOUT = sensorsOUT.getTempCByIndex(0);
    tempOUTReady = true;
  }
  if (sensorsUT.getCheckForConversion()==true) {
    tempUTReady = true;
    for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
      tempUT[i]=sensorsUT.getTempCByIndex(i);
    }
  }
}

void displayTemp() {
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