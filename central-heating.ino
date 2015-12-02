#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQTT.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <Base64.h>

//wiring
#define ONE_WIRE_BUS_IN                     13
#define ONE_WIRE_BUS_OUT                    2
#define ONE_WIRE_BUS_UT                     12

#define RELAYPIN                            14
#define LEDPIN                              0
#define BUZZERPIN                           16


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
bool                  relay         = LOW;
/*float                 tempON        = 40.f;
float                 tempOFF       = tempON - 5;
float                 tempOVER      = 100.f;
*/
String instanceId                   = "564241f9cf045c757f7e6301";
String valueStr("");
boolean result;
String topic("");
boolean switchState;
bool stepOk                         = false;

unsigned int const SERIAL_SPEED=115200;

#define AP_SSID "Datlovo"
#define AP_PASSWORD "Nu6kMABmseYwbCoJ7LyG"

//#define EIOT_CLOUD_TEMP_OUT_INSTANCE_PARAM_ID    "564241f9cf045c757f7e6301/SuCtZtHfuQzt6xap"
//#define EIOT_CLOUD_TEMP_IN_INSTANCE_PARAM_ID     "564241f9cf045c757f7e6301/VpZzAP0PegsWJeZO"
//#define EIOT_CLOUD_RELAY_INSTANCE_PARAM_ID       "564241f9cf045c757f7e6301/Ca9M05QD3xO3AuVP"
//#define REPORT_INTERVAL 60 // in sec
//#define EIOT_CLOUD_ADDRESS     "cloud.iot-playground.com"
//#define EIOT_CLOUD_PORT        40404

#define EIOTCLOUD_USERNAME "datel"
#define EIOTCLOUD_PASSWORD "mrdatel"

// create MQTT object
#define EIOT_CLOUD_ADDRESS        "cloud.iot-playground.com"
MQTT myMqtt("", EIOT_CLOUD_ADDRESS, 1883);

#define CONFIG_START 0
#define CONFIG_VERSION "v03"

struct StoreStruct {
  // This is for mere detection if they are your settings
  char    version[4];
  // The variables of your settings
  uint    moduleId;  // module id
  bool    relay;     // relay state
  float   tempON;
  float   tempOFFDiff;
  float   tempAlarm;
} storage = {
  CONFIG_VERSION,
  // The default module 0
  0,
  0, // off
  50,
  45,
  100
};


/*
// EasyIoT server definitions
#define EIOT_USERNAME    "admin"
#define EIOT_PASSWORD    "test"
#define EIOT_IP_ADDRESS  "192.168.1.53"
#define EIOT_PORT        80
#define EIOT_NODE        "N20S0"
#define USER_PWD_LEN 40
char unameenc[USER_PWD_LEN];
*/


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
float const   versionSW                   = 0.2;
char  const   versionSWString[]           = "Central heat v"; 


void setup(void)
{
  delay(2000);
  Serial.begin(SERIAL_SPEED);
  Serial.print(versionSWString);
  Serial.println(versionSW);

  EEPROM.begin(512);
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
  pinMode(BUZZERPIN, OUTPUT);
/*  Serial.print("Rele:");
  Serial.println(storage.relay);
  digitalWrite(RELAYPIN,storage.relay);
  digitalWrite(LEDPIN,storage.relay);
*/
  wifiConnect();
  
  /*char uname[USER_PWD_LEN];
  String str = String(EIOT_USERNAME)+":"+String(EIOT_PASSWORD);  
  str.toCharArray(uname, USER_PWD_LEN); 
  memset(unameenc,0,sizeof(unameenc));
  base64_encode(unameenc, uname, strlen(uname));
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
  
  Serial.println();
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
  Serial.print("Send interval ");
  Serial.print(sendDelay);
  Serial.println(" sec");
  Serial.print("Temp ON ");
  Serial.println(storage.tempON);
  Serial.print("Temp OFF diff ");
  Serial.println(storage.tempOFFDiff);
  Serial.print("Temp Over ");
  Serial.println(storage.tempAlarm);

  setMQTT();
 
  wdt_enable(WDTO_8S);
  
  lastSend=millis();
}


void loop(void)
{ 
  if (millis() - lastMeas >= measDelay) {
    lastMeas = millis();
    startMeas();    
    delay(measTime);
    getTemp();
    displayTemp();
  }
  
  if (millis() - lastSend >= sendDelay) {
    lastSend = millis();
    sendParamMQTT();
  }

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
  
  if ((tempOUT || tempIN) >= storage.tempAlarm) {
    digitalWrite(BUZZERPIN,HIGH);
  } else {
    digitalWrite(BUZZERPIN,LOW);
  }
}


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
}

/*
void sendParam(byte param)
{  
   WiFiClient client;
   
   while(!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT)) {
    Serial.println("connection failed");
    wifiConnect(); 
  }
 
  String url = "";
  if (param==OUT) {
    url += "/RestApi/SetParameter/"+ String(EIOT_CLOUD_TEMP_OUT_INSTANCE_PARAM_ID) + "/"+String(tempOUT); // generate EasIoT cloud update parameter URL
  }
  if (param==IN) {
    url += "/RestApi/SetParameter/"+ String(EIOT_CLOUD_TEMP_IN_INSTANCE_PARAM_ID) + "/"+String(tempIN); // generate EasIoT cloud update parameter URL
  }
  
  Serial.print("POST data to URL: ");
  Serial.println(url);
  
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + String(EIOT_CLOUD_ADDRESS) + "\r\n" + 
               "Connection: close\r\n" + 
               "Content-Length: 0\r\n" + 
               "\r\n");

  delay(100);
    while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("Connection closed");
}
*/

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

  EEPROM.commit();
}

void waitOk()
{
  while(!stepOk)
    delay(100);
 
  stepOk = false;
}

String macToStr(const uint8_t* mac)
{
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
  myMqtt.subscribe("/Db/"+instanceId+"/3/Sensor.Parameter1");
  myMqtt.subscribe("/Db/"+instanceId+"/4/Sensor.Parameter1");
  myMqtt.subscribe("/Db/"+instanceId+"/5/Sensor.Parameter1");
  myMqtt.subscribe("/Db/"+instanceId+"/6/Sensor.Parameter1");
  Serial.println(" OK.");
}