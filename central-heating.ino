#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <ESP8266WiFi.h>
#include <Base64.h>

//wiring
#define ONE_WIRE_BUS_IN                     13
#define ONE_WIRE_BUS_OUT                    2
#define ONE_WIRE_BUS_UT                     12

#define RELAYPIN                            14
#define LEDPIN                              0
#define BUZZERPIN                           16

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
const unsigned long   sendDelay     = 60000; //in ms
unsigned long         lastSend      = sendDelay;
float                 tempOUT       = 0.f;
float                 tempIN        = 0.f;
float                 tempUT[10];
float                 tempON        = 24.f;
float                 tempOFF       = tempON - 2;
float                 tempOVER      = 100.f;

//HIGH - relay OFF, LOW - relay ON
bool relay                          = HIGH; 

unsigned int const SERIAL_SPEED=115200;

#define AP_SSID                               "Datlovo"
#define AP_PASSWORD                           "Nu6kMABmseYwbCoJ7LyG"

#define EIOT_CLOUD_TEMP_INSTANCE_PARAM_ID     "564241f9cf045c757f7e6301/NTjETbUl91Ek0MB2"
#define REPORT_INTERVAL                       60 // in sec
#define EIOT_CLOUD_ADDRESS                    "cloud.iot-playground.com"
#define EIOT_CLOUD_PORT                       40404

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
  
  wifiConnect();
  
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);
  
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
  Serial.println(tempON);
  Serial.print("Temp OFF ");
  Serial.println(tempOFF);
  Serial.print("Temp Over ");
  Serial.println(tempOVER);


  wdt_enable(WDTO_8S);
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
    sendTeperature(tempOUT);
  }

  if (tempOUT >= tempON) {
    //Serial.println("Relay ON");
    relay = HIGH;
    
  }
  if (tempOUT <= tempOFF) {
    //Serial.println("Relay OFF");
    relay = LOW;
  }
  digitalWrite(RELAYPIN,relay);
  digitalWrite(LEDPIN,relay);
  
  if ((tempOUT || tempIN) >= tempOVER) {
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

void sendTeperature(float temp)
{  
   WiFiClient client;
   
   while(!client.connect(EIOT_CLOUD_ADDRESS, EIOT_CLOUD_PORT)) {
    Serial.println("connection failed");
    wifiConnect(); 
  }
 
  String url = "";
  url += "/RestApi/SetParameter/"+ String(EIOT_CLOUD_TEMP_INSTANCE_PARAM_ID) + "/"+String(temp); // generate EasIoT cloud update parameter URL
  
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