// Rotační enkodér KY-040

// proměnné pro nastavení propojovacích pinů
int pinCLK = 3;
int pinDT  = 4;
//int pinSW  = 5;

// proměnné pro uložení pozice a stavů pro určení směru
// a stavu tlačítka
int poziceEnkod = 0;
int stavPred;
int stavCLK;
int stavSW;

void setup() {
  Serial.begin(115200);
  Serial.println("Ventil");
  // nastavení propojovacích pinů jako vstupních
  pinMode(pinCLK, INPUT);
  pinMode(pinDT, INPUT);
  // nastavení propojovacího pinu pro tlačítko
  // jako vstupní s pull up odporem
  //pinMode(pinSW, INPUT_PULLUP);
  // načtení aktuálního stavu pinu CLK pro porovnávání
  stavPred = digitalRead(pinCLK);   
}

void loop() {
// načtení stavu pinu CLK
  stavCLK = digitalRead(pinCLK);
  // pokud je stav CLK odlišný od předchozího měření,
  // víme, že osa byla otočena
  if (stavCLK != stavPred) {
    // pokud stav pinu DT neodpovídá stavu pinu CLK,
    // byl pin CLK změněn jako první a rotace byla
    // po směru hodin, tedy vpravo
    if (digitalRead(pinDT) != stavCLK) {
      // vytištění zprávy o směru rotace a přičtení
      // hodnoty 1 u počítadla pozice enkodéru
      Serial.print("Rotace vpravo => | ");
      poziceEnkod ++;
    }
    // v opačném případě, tedy pin DT byl změněn
    // jako první, se jedná o rotaci
    // proti směru hodin, tedy vlevo
    else {
      // vytištění zprávy o směru rotace a odečtení
      // hodnoty 1 u počítadla pozice enkodéru
      Serial.print("Rotace vlevo  <= | ");
      poziceEnkod--;
    }
    // vytištění aktuální hodnoty pozice enkodéru
    Serial.print("Pozice enkoderu: ");
    Serial.println(poziceEnkod);
  }
  // uložení posledního stavu pinu CLK
  // jako reference pro další porovnávání
  stavPred = stavCLK;
}