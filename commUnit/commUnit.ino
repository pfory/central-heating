#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFiManager.h> 

const char *ssid = "Datlovo";
const char *password = "Nu6kMABmseYwbCoJ7LyG";

#define AIO_SERVER      "192.168.1.56"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

WiFiClient client;
WiFiManager wifiManager;

byte heartBeat                    = 12;
String received                   = "";

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

float versionSW                  = 0.9;
String versionSWString            = "Central Heating v";

void setup() {
  Serial.begin(SERIALSPEED);
  Serial.print(versionSWString);
  Serial.print(versionSW);
  
  //WiFi.begin(ssid, password);
  //wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  //WiFi.begin(ssid, password);
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
  pinMode(pinLed,OUTPUT); 
  digitalWrite(pinLed,LOW);
  delay(1000);
  digitalWrite(pinLed,HIGH);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  if (Serial.available()>0) { 
    received=Serial.readStringUntil('*');
    Serial.println(received);
  // }

  // if (millis() - lastSendTime >= sendTimeDelay) {
    // lastSendTime = millis();
    
    float temp[15];
    float tempINKamna, tempOUTKamna;
    int pumpStatus=0;
    
    bool emptyData=false;
    Serial.println(received);
    
    //received="#0;59.81#1;32.88#2;48.44#3;30.44#4;8.94#5;8.69#6;17.06#7;14.06#8;36.44#9;36.44#A;11.06#B;11.13#I;55.13#O;62.44#R;0$*";
    received.trim();
    if (received!="") {
      digitalWrite(pinLed,LOW);
      byte i=1;
      while (i<=15) {
        String val = getValue(received, '#', i);
        if (val.substring(0,1)=="0") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="1") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="2") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="3") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="4") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="5") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="6") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="7") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="8") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="9") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="A") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="B") {
          temp[i-1]=val.substring(2).toFloat(); 
          Serial.println(temp[i-1]);
        }
        if (val.substring(0,1)=="I") {
          tempINKamna=val.substring(2).toFloat(); 
          Serial.println(tempINKamna);
        }
        if (val.substring(0,1)=="O") {
          tempOUTKamna=val.substring(2).toFloat(); 
          Serial.println(tempOUTKamna);
        }
        if (val.substring(0,1)=="R") {
          pumpStatus=val.substring(2).toInt(); 
          Serial.println(pumpStatus);
        }
        i++;
      }
      Serial.println("I am sending data from Central heating unit to OpenHab");
    
      MQTT_connect();
      if (! tINKamna.publish(tempINKamna)) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! tOUTKamna.publish(tempOUTKamna)) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! sPumpKamna.publish(pumpStatus)) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      

      if (! t0.publish(temp[0])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t1.publish(temp[1])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t2.publish(temp[2])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t3.publish(temp[3])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t4.publish(temp[4])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t5.publish(temp[5])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t6.publish(temp[6])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t7.publish(temp[7])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t8.publish(temp[8])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t9.publish(temp[9])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t10.publish(temp[10])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! t11.publish(temp[11])) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! version.publish(versionSW)) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (! hb.publish(heartBeat++)) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
      if (heartBeat>1) {
        heartBeat=0;
      }
      digitalWrite(pinLed,HIGH);
    } else {
      emptyData=true;
      Serial.println("empty data");
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

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
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