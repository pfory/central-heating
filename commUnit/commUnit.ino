//ESP8266-01
//kompilovat jako Generic ESP8266 Module

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFiManager.h> 

#define AIO_SERVER      "192.168.1.56"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

#define verbose
#ifdef verbose
 #define DEBUG_PRINT(x)         Serial.print (x)
 #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)       Serial.println (x)
 #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
 #define PORTSPEED 115200
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTF(x, y)
#endif 

//for LED status
#include <Ticker.h>
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

WiFiClient client;
WiFiManager wifiManager;

uint32_t heartBeat                    = 12;
String received                       = "";
unsigned long milisLastRunMinOld      = 0;

IPAddress _ip           = IPAddress(192, 168, 1, 109);
IPAddress _gw           = IPAddress(192, 168, 1, 1);
IPAddress _sn           = IPAddress(255, 255, 255, 0);

#define pinLed                    2

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

unsigned int const sendTimeDelay  = 10000;
signed long lastSendTime          = sendTimeDelay * -1;

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish version             = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/VersionSWCentral");
Adafruit_MQTT_Publish hb                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/HeartBeat");
Adafruit_MQTT_Publish tINKamna            = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/tINKamna");
Adafruit_MQTT_Publish tOUTKamna           = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/tOUTKamna");
Adafruit_MQTT_Publish sPumpKamna          = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/sPumpKamna/status");
Adafruit_MQTT_Publish t0                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t0");
Adafruit_MQTT_Publish t1                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t1");
Adafruit_MQTT_Publish t2                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t2");
Adafruit_MQTT_Publish t3                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t3");
Adafruit_MQTT_Publish t4                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t4");
Adafruit_MQTT_Publish t5                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t5");
Adafruit_MQTT_Publish t6                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t6");
Adafruit_MQTT_Publish t7                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t7");
Adafruit_MQTT_Publish t8                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t8");
Adafruit_MQTT_Publish t9                  = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t9");
Adafruit_MQTT_Publish t10                 = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t10");
Adafruit_MQTT_Publish t11                 = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/t11");

#define SERIALSPEED 9600

void MQTT_connect(void);

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

float versionSW                  = 0.92;
String versionSWString            = "Central Heating v";

void setup() {
#ifdef verbose  
  Serial.begin(SERIALSPEED);
#endif
  DEBUG_PRINT(versionSWString);
  DEBUG_PRINT(versionSW);
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
  
  DEBUG_PRINTLN(ESP.getResetReason());
  if (ESP.getResetReason()=="Software/System restart") {
    heartBeat=1;
  } else if (ESP.getResetReason()=="Power on") {
    heartBeat=2;
  } else if (ESP.getResetReason()=="External System") {
    heartBeat=3;
  } else if (ESP.getResetReason()=="Hardware Watchdog") {
    heartBeat=4;
  } else if (ESP.getResetReason()=="Exception") {
    heartBeat=5;
  } else if (ESP.getResetReason()=="Software Watchdog") {
    heartBeat=6;
  } else if (ESP.getResetReason()=="Deep-Sleep Wake") {
    heartBeat=7;
  }

  wifiManager.setConnectTimeout(600); //5min

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
  //WiFi.config(ip); 
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    DEBUG_PRINTLN("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  if (Serial.available()>0) { 
    received=Serial.readStringUntil('*');
    DEBUG_PRINTLN(received);
  // }

  // if (millis() - lastSendTime >= sendTimeDelay) {
    // lastSendTime = millis();
    
    float temp[15];
    float tempINKamna, tempOUTKamna;
    int pumpStatus=0;
    
    bool emptyData=false;
    DEBUG_PRINTLN(received);
    
    //received="#0;59.81#1;32.88#2;48.44#3;30.44#4;8.94#5;8.69#6;17.06#7;14.06#8;36.44#9;36.44#A;11.06#B;11.13#I;55.13#O;62.44#R;0$*";
    received.trim();
    if (received!="") {
      digitalWrite(pinLed,LOW);
      byte i=1;
      while (i<=15) {
        String val = getValue(received, '#', i);
        if (val.substring(0,1)=="0") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="1") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="2") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="3") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="4") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="5") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="6") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="7") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="8") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="9") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="A") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="B") {
          temp[i-1]=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(temp[i-1]);
        }
        if (val.substring(0,1)=="I") {
          tempINKamna=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(tempINKamna);
        }
        if (val.substring(0,1)=="O") {
          tempOUTKamna=val.substring(2).toFloat(); 
          DEBUG_PRINTLN(tempOUTKamna);
        }
        if (val.substring(0,1)=="R") {
          pumpStatus=val.substring(2).toInt(); 
          DEBUG_PRINTLN(pumpStatus);
        }
        i++;
      }
      DEBUG_PRINTLN("I am sending data from Central heating unit to OpenHab");
    
      MQTT_connect();
      if (! tINKamna.publish(tempINKamna)) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! tOUTKamna.publish(tempOUTKamna)) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! sPumpKamna.publish(pumpStatus)) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      

      if (! t0.publish(temp[0])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t1.publish(temp[1])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t2.publish(temp[2])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t3.publish(temp[3])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t4.publish(temp[4])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t5.publish(temp[5])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t6.publish(temp[6])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t7.publish(temp[7])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t8.publish(temp[8])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t9.publish(temp[9])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t10.publish(temp[10])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      if (! t11.publish(temp[11])) {
        DEBUG_PRINTLN("failed");
      } else {
        DEBUG_PRINTLN("OK!");
      }
      
      if (millis() - milisLastRunMinOld > 60000) {
        if (! version.publish(versionSW)) {
          DEBUG_PRINTLN("failed");
        } else {
          DEBUG_PRINTLN("OK!");
        }
        if (! hb.publish(heartBeat)) {
          DEBUG_PRINTLN("failed");
        } else {
          DEBUG_PRINTLN("OK!");
        }
        heartBeat++;
      }
      
      digitalWrite(pinLed,HIGH);
    } else {
      emptyData=true;
      DEBUG_PRINTLN("empty data");
    }

  }
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  DEBUG_PRINT("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       DEBUG_PRINTLN(mqtt.connectErrorString(ret));
       DEBUG_PRINTLN("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  DEBUG_PRINTLN("MQTT Connected!");
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}