#include "beep.h"

#if ARDUINO >= 100
    #include "Arduino.h"   
#else
extern "C" {
    #include "WConstants.h"
}
#endif

Beep::Beep(uint8_t _pin) {
  pin=_pin;
  pinMode(pin, OUTPUT);
  beepStatus=255;
}

void Beep::noDelay(int _lengthBeep, int _lengthPause, byte _count, byte _volume) {
  if (beepStatus==255) {
    beepStart=millis();
    lengthBeep=_lengthBeep;
    lengthPause=_lengthPause;
    volume=_volume;
    beepCount=_count;
    beepStatus=1;
    beeped=0;
  }
}

void Beep::Delay(int _lengthBeep, int _lengthPause, byte _count, byte _volume) {
  for (uint8_t i=0; i<_count; i++) {
    digitalWrite(pin, 1);
    delay(_lengthBeep);
    digitalWrite(pin, 0);
    delay(_lengthPause);
  }
}

void Beep::loop() {
  stateMachine();
}

void Beep::stateMachine() {
  if (beepStatus==255) {
    return;
  }
  if (beepStatus==1) { //start
    analogWrite(pin, volume);
    beepStatus=2;
  } else if (beepStatus==0) {
    digitalWrite(pin, LOW);
    if (beepCount>1) {
      pauseStart=millis();
      beepStatus=3;
      beeped++;
    }
    if (beepCount<=beeped) {
      beepStatus=255;
      beepCount=0;
    }
  }else if (beepStatus==2) { //check beep delay
    if (millis()-beepStart>lengthBeep) {
      beepStatus=0;
    }
  } else if (beepStatus==3) { //check pause delay
    if (millis()-pauseStart>lengthPause) {
      if (beepCount>1) {
        beepStart=millis();
        beepStatus=1;
      }
    }
  }
}  


