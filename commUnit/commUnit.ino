#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

const char *ssid = "Datlovo";
const char *password = "Nu6kMABmseYwbCoJ7LyG";

#define AIO_SERVER      "192.168.1.56"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

WiFiClient client;

byte heartBeat=0;
unsigned long sendTimeDelay   = 60000;
String received               = "";

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

unsigned int const sendTimeDelay=10000;
signed long lastSendTime = sendTimeDelay * -1;

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish VersionSWCentral    = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/VersionSWCentral");
Adafruit_MQTT_Publish heartBeat           = Adafruit_MQTT_Publish(&mqtt, "home/Corridor/esp06/HeartBeat");
Adafruit_MQTT_Publish tINKamna            = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/tINKamna");
Adafruit_MQTT_Publish tOUTKamna           = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/tOUTKamna");
Adafruit_MQTT_Publish sPumpKamna/status   = Adafruit_MQTT_Publish(&mqtt, "/home/Corridor/esp06/sPumpKamna/status");
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

#define SERIALSPEED 115200

void MQTT_connect(void);

versionSW                  = "0.9"
versionSWString            = "Central Heating v" 

void setup() {
  Serial.begin(SERIALSPEED);
  Serial.print(versionSWString);
  Serial.print(versionSW);
  
  WiFi.begin(ssid, password);

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
  
  mqtt.subscribe(&com);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

 if (millis() - lastSendTime >= sendTimeDelay) {
    lastSendTime = millis();
    
    bool emptyData=false;
    Serial.println(received)
    
 
    received="#0;59.81#1;32.88#2;48.44#3;30.44#4;8.94#5;8.69#6;17.06#7;14.06#8;36.44#9;36.44#A;11.06#B;11.13#I;55.13#O;62.44#R;0$*"
    received=received.trim();
    if (received!="") {
    } else {
      emptyData=true;
      Serial.println("empty data")
    }
    
    if (emptyData) { 
    } else {
      Serial.println("I am sending data from Central heating unit to OpenHab");
    }
    
    if (! VersionSWCentral.publish(versionSW) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }



    if (! heartBeat.publish(heartBeat++)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }
    if (heartBeat>1) {
      heartBeat=0;
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