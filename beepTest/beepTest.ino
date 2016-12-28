#include "beep.h"

const byte pin=13;
byte volume=255;
int beepLength=100;
int pauseLength=100;
byte count=1;

Beep beep(pin);

void setup() {
  Serial.begin(9600);
  beep.Delay(beepLength, pauseLength, count, volume);
}

void loop() {
  beep.loop();
  if (Serial.available() > 0) {
    byte incomingByte = Serial.read();
    if (incomingByte==97) { //a
      volume+=10;
    } else if (incomingByte==115) { //s
      volume-=10;
    } else if (incomingByte==119) { //w
      beepLength+=10;
    } else if (incomingByte==122) { //z
      beepLength-=10;
    } else if (incomingByte==43) { //+
      pauseLength+=10;
    } else if (incomingByte==45) { //-
      pauseLength-=10;
    } else if (incomingByte==114) { //repeat
    } else {
      count = incomingByte-48;
    }
    beep.noDelay(beepLength, pauseLength, count, volume);
    Serial.print(beepLength);
    Serial.print(",");
    Serial.print(pauseLength);
    Serial.print(",");
    Serial.print(count);
    Serial.print(",");
    Serial.println(volume);
    
  }
  //delay(5000);
}
