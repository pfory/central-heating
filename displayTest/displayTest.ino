#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#define LCDADDRESS   0x27
#define EN           2
#define RW           1
#define RS           0
#define D4           4
#define D5           5
#define D6           6
#define D7           7
#define BACKLIGHT    3
#define POL          POSITIVE
#define LCDROWS      4
#define LCDCOLS      20
LiquidCrystal_I2C lcd(LCDADDRESS,EN,RW,RS,D4,D5,D6,D7,BACKLIGHT,POL);  // set the LCD

#define RELAYPIN                            6

//-------------------------------------------- S E T U P ------------------------------------------------------------------------------
void setup() {
  lcd.begin(LCDCOLS,LCDROWS);               // initialize the lcd 
  lcd.setBacklight(255);
  lcd.clear();
  lcd.print("Test displeje");
  delay(500);
  lcd.clear();
  
  pinMode(RELAYPIN, OUTPUT);
   
  randomSeed(analogRead(0));
}
/*
  01234567890123456789
  --------------------
0|xxx/xxx     15:30:45
1|1:xxx/xxx 2:xxx/xxx 
2|3:xxx/xxx 4:xxx/xxx 
3|5:xxx/xxx 6:xxx/xxx 
  --------------------
  01234567890123456789
*/

void loop() {
  lcd.setCursor(0, 0); //col,row
  lcd.print("       ");
  lcd.setCursor(0, 0); //col,row
  float randNumber = random(-15, 110);
  addSpaces(randNumber);
  lcd.print((int)randNumber);
  lcd.setCursor(3, 0); //col,row
  lcd.print("/");
  randNumber = random(-15, 110);
  addSpaces(randNumber);
  lcd.print((int)randNumber);
  lcd.setCursor(12, 0); //col,row
  lcd.print("15:30:45");

  for (byte i=0; i<6; i=i+2) {
    lcd.setCursor(0, i/2+1);
    lcd.print(i+1);
    lcd.print(":       ");
    lcd.setCursor(2, i/2+1);
    randNumber = random(-15, 110);
    addSpaces(randNumber);
    lcd.print((int)randNumber);    
    lcd.setCursor(5, i/2+1);
    lcd.print("/");
    randNumber = random(-15, 110);
    addSpaces(randNumber);
    lcd.print((int)randNumber);    
    lcd.setCursor(10, i/2+1);
    lcd.print(i+2);
    lcd.print(":       ");
    lcd.setCursor(12, i/2+1);
    randNumber = random(-15, 110);
    addSpaces(randNumber);
    lcd.print((int)randNumber);    
    lcd.setCursor(15, i/2+1);
    lcd.print("/");
    randNumber = random(-15, 110);
    addSpaces(randNumber);
    lcd.print((int)randNumber);  
  }

  byte relay=(int)random(0, 2);
  digitalWrite(RELAYPIN,relay);
  lcd.setCursor(8, 0);
  if (relay==0) {
    lcd.print("    ");
  }else{
    lcd.print("CER");
  }
  
  delay(5000);
}

void addSpaces(int cislo) {
  if (cislo<100 && cislo>0) lcd.print(" ");
  if (cislo<10 && cislo>0) lcd.print(" ");
  if (cislo<=0 && cislo>-10) lcd.print(" ");
}