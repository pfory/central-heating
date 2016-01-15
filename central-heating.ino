//Rad1 - WorkRoom
//Rad2 - LivingRoom OUT
//Rad3 - BedroomOld
//Rad4 - BedRoomNew
//Rad5 - LivingRoom IN
//Rad6 - 
//Rad7 - BedroomOld
//Rad8 - BedRoomNew

#define arduino

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "beep.h"

#ifdef arduino
#include <EEPROM.h>
#include <avr/wdt.h>
#endif
#ifndef arduino
/*
#include <MQTT.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>*/
#endif

//wiring
#ifdef arduino
#define ONE_WIRE_BUS_IN                     2
#define ONE_WIRE_BUS_OUT                    4
#define ONE_WIRE_BUS_UT                     5
#define RELAYPIN                            6
#define LEDPIN                              13
#define BUZZERPIN                           8
#else
#define ONE_WIRE_BUS_IN                     13
#define ONE_WIRE_BUS_OUT                    2
#define ONE_WIRE_BUS_UT                     12
#define RELAYPIN                            14
#define LEDPIN                              0
#define BUZZERPIN                           16
#endif

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
float                 tempUT[12];
bool                  relay         = LOW;
#ifndef arduino
String instanceId                   = "564241f9cf045c757f7e6301";
String valueStr("");
boolean result;
String topic("");
bool stepOk                         = false;
byte status                         = 0;

#define AP_SSID "Datlovo"
#define AP_PASSWORD "Nu6kMABmseYwbCoJ7LyG"

#define EIOTCLOUD_USERNAME "datel"
#define EIOTCLOUD_PASSWORD "mrdatel"

// create MQTT object
#define EIOT_CLOUD_ADDRESS        "cloud.iot-playground.com"
MQTT myMqtt("", EIOT_CLOUD_ADDRESS, 1883);
#endif

unsigned int const SERIAL_SPEED=9600;  //kvuli BT modulu jinak muze byt vice

#define CONFIG_START 0
#define CONFIG_VERSION "v03"

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
  100
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

//SW name & version
float const   versionSW                   = 0.3;
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

#ifndef arduino
  EEPROM.begin(512);
#endif
  loadConfig();
  /*
  lcd.home();                   // go home
  lcd.print(versionSWString);  
  lcd.print(" ");
  lcd.print (versionSW);
  delay(1000);
  lcd.clear();
  */
  
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
/*  Serial.print("Rele:");
  Serial.println(storage.relay);
  digitalWrite(RELAYPIN,storage.relay);
  digitalWrite(LEDPIN,storage.relay);
*/
  
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

  delay(5000);
  lcd.clear();

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
  Serial.print("Temp Over ");
  Serial.println(storage.tempAlarm);

#ifndef arduino
  wifiConnect();
  setMQTT();
  
  valueStr = String(STATUS_AFTER_BOOT);
  topic = "/Db/" + instanceId + "/1/Sensor.Status";
  result = myMqtt.publish(topic, valueStr);
#endif  
  wdt_enable(WDTO_8S);
  
  lastSend=0;
}

/////////////////////////////////////////////   L  O  O  P   ///////////////////////////////////////
void loop(void) { 
  wdt_reset();
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    startMeas();    
    delay(measTime);
    getTemp();
    printTemp();
    displayTemp();
  }
  
#ifndef arduino
  if (millis() - lastSend >= sendDelay) {
    lastSend = millis();
    sendParamMQTT();
  }
#endif
  if (tempOUT <= storage.tempON - storage.tempOFFDiff) {
    //Serial.println("Relay OFF");
    relay = LOW;
  }
  if ((tempOUT >= storage.tempON) || (tempIN >= storage.tempON)) {
    //Serial.println("Relay ON");
    relay = HIGH;
  }
  
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,relay);
  
  if ((tempOUT || tempIN) >= storage.tempAlarm && (tempOUT || tempIN) < storage.tempAlarm+5) {
    beep.noDelay(100,40,3,5);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+5 && (tempOUT || tempIN) < storage.tempAlarm+10) {
    beep.noDelay(100,40,5,230);
  } else if ((tempOUT || tempIN) >= storage.tempAlarm+10) {
    beep.noDelay(100,40,9,255);
  }

  beep.loop();
  sevseg.refreshDisplay(); // Must run repeatedly; don't use blocking code (ex: delay()) in the loop() function or this won't work right
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
  if (sensorsIN.getCheckForConversion()==true) {
    tempIN = sensorsIN.getTempCByIndex(0);
  }
  if (sensorsOUT.getCheckForConversion()==true) {
    tempOUT = sensorsOUT.getTempCByIndex(0);
  }
  if (sensorsUT.getCheckForConversion()==true) {
    for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
      tempUT[i]=sensorsUT.getTempCByIndex(i);
    }
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

#ifndef arduino
void wifiConnect()
{
    Serial.print("Connecting to AP");
    WiFi.begin(AP_SSID, AP_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");  
}

void sendParamMQTT() {
  valueStr = String(tempOUT);
  topic = "/Db/" + instanceId + "/1/Sensor.TempOUT";
  result = myMqtt.publish(topic, valueStr);
  valueStr = String(tempIN);
  topic = "/Db/" + instanceId + "/1/Sensor.TempIN";
  result = myMqtt.publish(topic, valueStr);
  valueStr = String(relay);
  topic = "/Db/" + instanceId + "/1/Sensor.Pump";
  result = myMqtt.publish(topic, valueStr);
  //teploty radiatoru
  for (byte i=0; i<sensorsUT.getDeviceCount(); i++) {
    valueStr = String(tempUT[i]);
    topic = "/Db/" + instanceId + "/1/Sensor.TempRad" + String(i+1);
    result = myMqtt.publish(topic, valueStr);
    //Serial.print(topic);
    //Serial.println(valueStr);
  }
  valueStr = String(status++);
  topic = "/Db/" + instanceId + "/1/Sensor.Status";
  result = myMqtt.publish(topic, valueStr);
  if (status==2) status =0;
}
#endif

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
  for (unsigned int t=0; t<sizeof(storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
#ifndef arduino
  EEPROM.commit();
#endif
}

#ifndef arduino
void waitOk()
{
  while(!stepOk)
    delay(100);
 
  stepOk = false;
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}


void myConnectedCb() {
  Serial.println("connected to MQTT server");
}

void myDisconnectedCb() {
  Serial.println("disconnected. try to reconnect...");
  delay(500);
  myMqtt.connect();
}

void myPublishedCb() {
  Serial.println(" - published.");
}

void myDataCb(String& topic, String& data) {  
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(data);

  if (topic == String("/Db/InstanceId"))
  {
    //instanceId = data;
    stepOk = true;
  }
  else if (topic ==  String("/Db/"+instanceId+"/NewModule"))
  {
    storage.moduleId = data.toInt();
    stepOk = true;
  }
  else if (topic == String("/Db/"+instanceId+"/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter"))
  {
    stepOk = true;
  }
  else if (topic == String("/Db/"+instanceId+"/"+String(storage.moduleId)+ "/Settings.Icon1/NewParameter"))
  {
    stepOk = true;
/*  } else if (topic == String("/Db/"+instanceId+"/3/Sensor.Parameter1")) {
    storage.relay = (data == String("1"))? true: false;
    Serial.println("SAVE relay state");
    Serial.println(storage.relay);
    saveConfig();*/
  } else if (topic == String("/Db/"+instanceId+"/4/Sensor.Parameter1")) {
    storage.tempON = data.toFloat();
    Serial.print("SAVE TempON = ");
    Serial.println(storage.tempON);
    saveConfig();
  } else if (topic == String("/Db/"+instanceId+"/6/Sensor.Parameter1")) {
    storage.tempOFFDiff = data.toFloat();
    Serial.print("SAVE tempOFFDiff = ");
    Serial.println(storage.tempOFFDiff);
    saveConfig();
  } else if (topic == String("/Db/"+instanceId+"/5/Sensor.Parameter1")) {
    storage.tempAlarm = data.toFloat();
    Serial.print("SAVE TempAlarm = ");
    Serial.println(storage.tempAlarm);
    saveConfig();
  }
}

void setMQTT() {
  String clientName;
  //clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  myMqtt.setClientId((char*) clientName.c_str());


  Serial.print("MQTT client id:");
  Serial.println(clientName);

  // setup callbacks
  myMqtt.onConnected(myConnectedCb);
  myMqtt.onDisconnected(myDisconnectedCb);
  myMqtt.onPublished(myPublishedCb);
  myMqtt.onData(myDataCb);
  
  //////Serial.println("connect mqtt...");
  myMqtt.setUserPwd(EIOTCLOUD_USERNAME, EIOTCLOUD_PASSWORD);  
  myMqtt.connect();

  delay(500);
  
  //get instance id
  //////Serial.println("suscribe: Db/InstanceId");
  myMqtt.subscribe("/Db/InstanceId");

  waitOk();


  Serial.print("ModuleId: ");
  Serial.println(storage.moduleId);


  //create module if necessary 
  if (storage.moduleId == 0)
  {
    //create module

    Serial.println("create module: Db/"+instanceId+"/NewModule");

    myMqtt.subscribe("/Db/"+instanceId+"/NewModule");
    waitOk();
      
    // create Sensor.Parameter1
    
    Serial.println("/Db/"+instanceId+"/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter");    

    myMqtt.subscribe("/Db/"+instanceId+"/"+String(storage.moduleId)+ "/Sensor.Parameter1/NewParameter");
    waitOk();

    // set module type
        
    Serial.println("Set module type");    

    valueStr = "MT_DIGITAL_OUTPUT";
    topic  = "/Db/" + instanceId + "/" + String(storage.moduleId) + "/ModuleType";
    result = myMqtt.publish(topic, valueStr);
    delay(100);

    // save new module id
    saveConfig();
  }

  
  //tempON
  valueStr = String(storage.tempON);
  topic  = "/Db/"+instanceId+"/4/Sensor.Parameter1";
  Serial.print("Publish " + topic);
  result = myMqtt.publish(topic, valueStr);
  delay(1000);
  //tempOFF
  valueStr = String(storage.tempOFFDiff);
  topic  = "/Db/"+instanceId+"/6/Sensor.Parameter1";
  Serial.print("Publish " + topic);
  result = myMqtt.publish(topic, valueStr);
  delay(1000);
  //tempOVER
  valueStr = String(storage.tempAlarm);
  topic  = "/Db/"+instanceId+"/5/Sensor.Parameter1";
  Serial.print("Publish " + topic);
  result = myMqtt.publish(topic, valueStr);
  delay(1000);

  
  //switchState = storage.state;
  Serial.print("Subscribe ");
  myMqtt.subscribe("/Db/"+instanceId+"/4/Sensor.Parameter1");
  myMqtt.subscribe("/Db/"+instanceId+"/5/Sensor.Parameter1");
  myMqtt.subscribe("/Db/"+instanceId+"/6/Sensor.Parameter1");
  Serial.println(" OK.");
}
#endif


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
  addSpaces((int)tempOUT);
  lcd.print((int)tempOUT);
  lcd.setCursor(12, 0); //col,row
  lcd.print("15:30:45");

  byte radka=1;
  byte sensor=0;
  for (byte i=0; i<sensorsUT.getDeviceCount(); i=i+4) {
    lcd.setCursor(0, radka);
    lcd.print(sensor+1);
    lcd.print(":       ");
    lcd.setCursor(2, radka);
    addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
    lcd.setCursor(5, radka);
    lcd.print("/");
    addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
    lcd.setCursor(10, radka);
    lcd.print(sensor);
    lcd.print(":       ");
    lcd.setCursor(12, radka);
    addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
    lcd.setCursor(15, radka++);
    lcd.print("/");
    addSpaces((int)tempUT[sensor]);
    lcd.print((int)tempUT[sensor++]);    
  }

  lcd.setCursor(8, 0);
  if (relay==LOW) {
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