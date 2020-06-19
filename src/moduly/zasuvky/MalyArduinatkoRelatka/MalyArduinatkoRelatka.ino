// Visual Micro is in vMicro>General>Tutorial Mode
//
/*
    Name:       MalyArduinatkoRelatka.ino
    Created:	25.10.2019 18:44:34
    Author:     DESKTOP-9UH2GP7\svita
*/
#define SRX 2
#define STX 3
#define R1 4
#define R2 5
#define R3 6
#define R4 7
#define R5 8
#define R6 9
#define R7 10
#define R8 11
#include <SoftwareSerial.h>
volatile bool relatka[8] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile bool uprava = false;
volatile bool odesliDoModulu = false;
unsigned long cas;
unsigned long ctiTlacidla;
unsigned long ctiModulos;
void nastavRka() {
  for (int i = 4; i <= 11; i++) {
    digitalWrite(i, !relatka[i - 4]);
  }
}

SoftwareSerial mySerial(SRX, STX); // RX, TX>

// The setup() function runs once each time the micro-controller starts
void setup()
{
  pinMode(A0, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A1, OUTPUT);
  digitalWrite(A1, LOW);
  digitalWrite(A0, HIGH);
  digitalWrite(A5, LOW);
  pinMode(A6, INPUT);
  pinMode(A4, INPUT);

  for (int i = 4; i <= 11; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }

  Serial.begin(115200);
  mySerial.begin(9600);
  cas = millis();
  ctiTlacidla = millis();
  mySerial.println("RX:?");

}
byte hodMiRelatkaDoCisla() {
  byte cislo = 0;
  for (byte i = 0; i < 8; i++) {
    cislo += relatka[i] << i;
  }
  return cislo;

}

// Add the main program code into the continuous loop() function
void loop()
{
  if (uprava) {
    Serial.println("nastavuji");
    nastavRka();
    uprava = false;
  }
  if (odesliDoModulu) {
    byte zapunty = hodMiRelatkaDoCisla();
    char posledniZparava[7];
    if (zapunty == 0) {
      sprintf(posledniZparava, "RX:0\n");
    }
    else {
      sprintf(posledniZparava, "P%c\n", zapunty);
    }
    mySerial.print(posledniZparava);
    odesliDoModulu = false;

  }
  if (millis() - cas >= 100) {
    cas = millis();
    if (Serial.available()) {
      byte n = Serial.available();
      char pole[n];
      for (byte i = 0; i < n; i++) {
        pole[i] = Serial.read();
        mySerial.write(pole[i]);
      }
      if (pole[n - 2] == 13 || pole[n - 2] == 10) pole[n - 2] = '\0';
      if (pole[n - 1] == 13 || pole[n - 1] == 10) pole[n - 1] = '\0';

      provokace(pole, n);
      if (uprava)Serial.println("yes");
    }
  }
  if (millis() - ctiModulos >= 50) {
    ctiModulos = millis();
    if (mySerial.available() > 0) //Checks is there any data in buffer
    {
      byte n = mySerial.available();
      char pole[n];
      Serial.print("#");
      for (byte i = 0; i < n; i++) {
        pole[i] = mySerial.read();
        Serial.write(pole[i]);
      }
      Serial.flush();
      if (pole[n - 2] == 13 || pole[n - 2] == 10) pole[n - 2] = '\0';
      if (pole[n - 1] == 13 || pole[n - 1] == 10) pole[n - 1] = '\0';

      provokace(pole, n);
      if (uprava) {
        mySerial.println("yes");
        Serial.println("asi jo");
      }
      char* radek = pole;
      for (byte i = 0; i < n; i++) {
      if (pole[i] == '\n') {
        radek=&pole[i+1];
          provokace(radek, n);
          if (uprava) {
            mySerial.println("yes");
            //Serial.println("asi jo");
            break;
          }
        }
      }

    }
  }
  if (millis() - ctiTlacidla >= 300) {
    ctiTlacidla = millis();
    int levastrana = analogRead(A4);
    int pravastrana = analogRead(A6);
    switch (levastrana) {
      case 20 ... 40: { //power
          relatka[0] = !relatka[0];
          uprava = true;
          break;
        }
      case 410 ... 420: { //open
          relatka[1] = !relatka[1];
          uprava = true;
          break;
        }
    }
    switch (pravastrana) {
      case 950 ... 970: { //pause
          relatka[2] = !relatka[2];
          uprava = true;
          break;
        }
      case 820 ... 830: { //skip-
          relatka[3] = !relatka[3];
          uprava = true;
          break;
        }
      case 740 ... 750: { //stop
          relatka[4] = !relatka[4];
          uprava = true;
          break;
        }
      case 610 ... 660: { //play
          relatka[5] = !relatka[5];
          uprava = true;
          break;
        }
      case 880 ... 889: { //skip+
          relatka[6] = !relatka[6];
          uprava = true;
          break;
        }
    }
    if (uprava) {
      odesliDoModulu = true;
    }

  }
}
void prevedNaPiny(byte cislo) {
  for (byte i = 0; i < 8; i++) {
    relatka[i] = 0b00000001 & cislo;
    cislo = cislo >> 1;
  }

}
void provokace(char pole[], byte n) {
  if (pole[0] == 'P') {
    prevedNaPiny(pole[1]);
    uprava = true;
    //Serial.println("jsem v pecku");
  }
  else if (pole[0] == 'R') {
    //Serial.println("jsem v reku");
    switch (pole[1] - 48) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8: {
          byte akce = 0;
          if (pole[3] == '1') { //formát zprávy Rx:0/1
            akce = 1;
            //Serial.printf("ano zapnu ti ho %s",pole);
          }
          if (akce != relatka[pole[1] - 48 - 1]) {
            relatka[pole[1] - 48 - 1] = akce;
            uprava = 1;
          }
          break;
        }
      case 40: {
          bool bylaZmena = 0;
          if (pole[3] == '1') {		 //RX:0/1
            for (byte pohoda = 0; pohoda < 8; pohoda++) {
              if (relatka[pohoda] != 1) {
                relatka[pohoda] = 1;
                bylaZmena = 1;
              }
            }
          }
          if (pole[3] == '0') {
            for (byte pohoda = 0; pohoda < 8; pohoda++) {
              if (relatka[pohoda] != 0) {
                relatka[pohoda] = 0;
                bylaZmena = 1;
              }
            }
          }
          uprava = bylaZmena;
          break;
        }
    }
    //Serial.print("asi ");
    //Serial.println(uprava);
  }
  else {
    //Serial.print("nic ");
   // Serial.print(pole[0]);
    //Serial.println("  konec vety");
  }
}
