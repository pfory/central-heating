char incomingByte = 0;   // for incoming serial data

void setup() {
  Serial.begin(9600);     // opens serial port, sets data rate to 9600 bps
  Serial2.begin(9600);     // opens serial port, sets data rate to 9600 bps
  randomSeed(analogRead(0));
}

void loop() {
  String s;
  s="#0;";
  s+=(float)random(0, 1100)/10;
  s+="#1;";
  s+=(float)random(0, 1100)/10;
  s+="#2;";
  s+=(float)random(0, 1100)/10;
  s+="#3;";
  s+=(float)random(0, 1100)/10;
  s+="#4;";
  s+=(float)random(0, 1100)/10;
  s+="#5;";
  s+=(float)random(0, 1100)/10;
  s+="#6;";
  s+=(float)random(0, 1100)/10;
  s+="#7;";
  s+=(float)random(0, 1100)/10;
  s+="#8;";
  s+=(float)random(0, 1100)/10;
  s+="#9;";
  s+=(float)random(0, 1100)/10;
  s+="#A;";
  s+=(float)random(0, 1100)/10;
  s+="#B;";
  s+=(float)random(0, 1100)/10;
  s+="#I;";
  s+=(float)random(0, 1100)/10;
  s+="#O;";
  s+=(float)random(0, 1100)/10;
  s+="#R;";
  s+=random(0, 1);
  s+="$3419268668*";

  Serial.println(s);
  Serial2.print(s);
  delay(20000);
}
 