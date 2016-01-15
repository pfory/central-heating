#include <SevSeg.h>
#include <wire.h>

SevSeg sevseg; //Instantiate a seven segment controller object
/*byte counter=0;
long lastMillis=0;
*/
byte temperature;

void setup()
{
  byte numDigits = 2;   
  byte digitPins[] = {10,11}; //Digits: 1,2,3,4 <--put one resistor (ex: 220 Ohms, or 330 Ohms, etc, on each digit pin)
  byte segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 0}; //Segments: A,B,C,D,E,F,G,Period

  sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins);
  sevseg.setBrightness(100); //Note: 100 brightness simply corresponds to a delay of 2000us after lighting each segment. A brightness of 0 
                            //is a delay of 1us; it doesn't really affect brightness as much as it affects update rate (frequency).
                            //Therefore, for a 4-digit 7-segment + pd, COMMON_ANODE display, the max update rate for a "brightness" of 100 is 1/(2000us*8) = 62.5Hz.
                            //I am choosing a "brightness" of 10 because it increases the max update rate to approx. 1/(200us*8) = 625Hz.
                            //This is preferable, as it decreases aliasing when recording the display with a video camera....I think.
  
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
}

void loop()
{
/*  if (millis()-lastMillis>=1000) {
    lastMillis=millis();
    sevseg.setNumber(counter++, 0);
    if (counter==100)
      counter=0;
  }
*/
  sevseg.setNumber(temperature, 0);
  sevseg.refreshDisplay(); // Must run repeatedly; don't use blocking code (ex: delay()) in the loop() function or this won't work right
}


void receiveEvent(int howMany) {
  temperature = Wire.read();    // receive byte as an integer
}
