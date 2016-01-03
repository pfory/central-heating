#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//wiring
#define ONE_WIRE_BUS_OUT                    2
OneWire oneWireOUT(ONE_WIRE_BUS_OUT);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorsOUT(&oneWireOUT);
float                 tempOUT[10];


unsigned int const SERIAL_SPEED=9600;


void setup(void)
{
  Serial.begin(SERIAL_SPEED);
  sensorsOUT.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  sensorsOUT.setResolution(12);
  sensorsOUT.setWaitForConversion(false);
  Serial.print("Sensor(s) ");
  Serial.print(sensorsOUT.getDeviceCount());
  Serial.print(" on bus OUT - pin ");
  Serial.println(ONE_WIRE_BUS_OUT);
}


void loop(void) { 
  startMeas();    
  delay(1000);
  getTemp();
  displayTemp();
}


void startMeas() {
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensorsOUT.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
}

void getTemp() {
  if (sensorsOUT.getCheckForConversion()==true) {
    for (byte i=0; i<sensorsOUT.getDeviceCount(); i++) {
      tempOUT[i]=sensorsOUT.getTempCByIndex(i);
    }
  }
}

void displayTemp() {
  for (byte i=0; i<sensorsOUT.getDeviceCount(); i++) {
    Serial.print("Temp UT[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.print(tempOUT[i]); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
    Serial.println();
  }
}
