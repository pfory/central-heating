#include <SevSeg.h>
#include <Wire.h>

SevSeg sevseg; //Instantiate a seven segment controller object
byte counter=0;
long lastMillis=0;

float temperature;

void setup() {
  Serial.begin(9600);
  byte numDigits = 4;   
  byte digitPins[] = {10,11,12,13}; //Digits: 1,2,3,4 <--put one resistor (ex: 220 Ohms, or 330 Ohms, etc, on each digit pin)
  byte segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 9}; //Segments: A,B,C,D,E,F,G,Period

  sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins);
  sevseg.setBrightness(100); //Note: 100 brightness simply corresponds to a delay of 2000us after lighting each segment. A brightness of 0 
                            //is a delay of 1us; it doesn't really affect brightness as much as it affects update rate (frequency).
                            //Therefore, for a 4-digit 7-segment + pd, COMMON_ANODE display, the max update rate for a "brightness" of 100 is 1/(2000us*8) = 62.5Hz.
                            //I am choosing a "brightness" of 10 because it increases the max update rate to approx. 1/(200us*8) = 625Hz.
                            //This is preferable, as it decreases aliasing when recording the display with a video camera....I think.
  
 
 

 
  digitalWrite(13, LOW);
}

void loop() {
  if (millis()-lastMillis>=100) {
    lastMillis=millis();
    sevseg.setNumber(temperature, 1);
    temperature+=0.1f;
    //Serial.println(temperature);
    
    /*Wire.requestFrom(8, 6);    // request 6 bytes from slave device #8
    while (Wire.available()) { // slave may send less than requested
      char c = Wire.read(); // receive a byte as character
      Serial.print(c);         // print the character
    }
    */
  }
  //sevseg.setNumber(temperature, counter);
  sevseg.refreshDisplay(); // Must run repeatedly; don't use blocking code (ex: delay()) in the loop() function or this won't work right
}

void receiveEvent(int howMany) {
  digitalWrite(13, HIGH);
  temperature = Wire.read();    // receive byte as an integer
  digitalWrite(13, LOW);
}