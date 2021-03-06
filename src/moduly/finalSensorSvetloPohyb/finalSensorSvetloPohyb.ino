#define jmeno 0
#define wifi 1
#define wifiHeslo 2
#define jmenoDB 3
#define PjmenoDB 4
#define PhesloDB 5
#define Maximu_clientu 10
#define mixim_delka_lajny 50
#define interval_ulozeni 300000

#include "ESP8266WiFi.h"
#include "EEPROM.h"
#include <Ticker.h>
#include <Wire.h>
#include <BH1750.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

BH1750 luxSenzor;

#define LED 2
const int motionSensor = D8;

WiFiServer wifiServer(8888);
#define pocetUdaju 6
const char* udaje[pocetUdaju];
char blaznivyNapad [pocetUdaju][32];
byte stavKomunikace = 0;
byte velikostEpromky = 206;//32 jmeno zarizeni 32 jmeno wifi 32 heslo wifi + 3 pokazde velikost pred obsahem promene  plus novinka 32 jmeno db 32 jmeno 32 heslo +4 ip serveru + 4 na port takze celk velikost 6*33 +4+4

WiFiClient nakladac;
MySQL_Connection conn((Client *)&nakladac);
byte  dbAdresa[4] = {0, 0, 0, 0};
IPAddress server_addr(88, 86, 120, 229); // IP of the MySQL *server* here
int port = 3310;
//pro zalozeni tabulky
const char zaloz[] = "USE %s; CREATE TABLE IF NOT EXISTS `%s` (   `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, `pohyb` BOOLEAN DEFAULT NULL,  `svetlo` MEDIUMINT UNSIGNED DEFAULT NULL,  `cas` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,PRIMARY KEY  (`id`)) ENGINE=MyISAM DEFAULT CHARSET=utf8;";
//"CREATE TABLE IF NOT EXISTS `%s`.`%s` ( `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT ,  `cas` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ,  `vlhkost` FLOAT NOT NULL ,  `teplota` FLOAT NOT NULL ,  `zprava` CHAR(40) NOT NULL ,    PRIMARY KEY  (`id`)) ENGINE = MyISAM;";
// Sample query
const char INSERT_DATA[] = "USE %s; INSERT INTO `%s` (pohyb, svetlo) VALUES (%d,%d);";
volatile byte oscanuj = 0;

Ticker blinker;

WiFiClient *clients[Maximu_clientu] = { NULL };
char inputs[Maximu_clientu][mixim_delka_lajny] = { 0 };
byte coPoMeKdoChce[Maximu_clientu] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long cas;
unsigned long casmereni;
volatile int pocetKlientu = 0;
volatile uint16_t lux = 0;
volatile bool pohyb = false;
volatile bool nastalaZmena = false;
volatile bool posledniPoslanaHodnotaDoDb = false;

void setup() {
  konfigurace_init();
  //blinker.attach(0.5, konfigurace);
  cas = millis();
  casmereni = millis();
  digitalWrite(LED, HIGH);
  zalozTabulku();
  pocetKlientu = 0;
}



void loop() {
  praceSklienty();
  if (millis() - cas >= interval_ulozeni) {
    Serial.println("jsem tady");
      uloz();
    /*if (WiFi.status() == WL_CONNECTED) {
      
    }
    else setup();*/
    cas = millis();
  }
  if (millis() - casmereni >= 5000) {
    casmereni = millis();
    nactiMiHodnoty();
  }
  if (oscanuj) {
    oscanuj = 0;
    scanujSite();
  }
  if (nastalaZmena) {
    nastalaZmena = false;
    zmenaRvi();
  }

}
//**********************************************************************************************************************************
//cteni ze senzoru
//**********************************************************************************************************************************
void nactiMiHodnoty() {
  uint16_t svetlost = luxSenzor.readLightLevel();
  if (svetlost != lux) {
    lux = svetlost;
    nastalaZmena = true;
  }
}
ICACHE_RAM_ATTR void detectsMovement() {
  bool stav = digitalRead(motionSensor);
  if (pohyb != stav) {
    pohyb = stav;
    //Serial.print("zmena:    ");
    //Serial.println(pohyb);
    nastalaZmena = true;
    if (millis() - cas >= 10000) {
      if (pohyb != posledniPoslanaHodnotaDoDb) {
        cas = millis() - interval_ulozeni;
      }
    }
  }
  //Serial.println("sem tu");

}
//**********************************************************************************************************************************
//ulozeni do db
//**********************************************************************************************************************************
void uloz() {
  Serial.println("Connecting...");
  if (conn.connect(server_addr, port, blaznivyNapad [PjmenoDB], blaznivyNapad [PhesloDB])) {
    delay(100);
    char query[255];
    // zprava co odešlu
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    // nactu hodnoty
    nactiMiHodnoty();
    posledniPoslanaHodnotaDoDb = pohyb;
    sprintf(query, INSERT_DATA,  blaznivyNapad [jmenoDB], blaznivyNapad[jmeno], posledniPoslanaHodnotaDoDb, lux);
    //Serial.println(query);
    // zapise do db
    cur_mem->execute(query);
    delete cur_mem;
    Serial.println("Data recorded.");
  }
  else
    Serial.println("Connection failed.");
  conn.close();
}
void zalozTabulku() {

  Serial.println("Connecting...");
  if (conn.connect(server_addr, port, blaznivyNapad [PjmenoDB], blaznivyNapad [PhesloDB])) {
    delay(100);
    char query[360];
    // zprava co odešlu
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    // nactu hodnoty
    //sprintf(query, zaloz, blaznivyNapad [jmenoDB], blaznivyNapad[jmeno]);
    //Serial.println(query);
    // zapise do db
    cur_mem->execute(query);
    delete cur_mem;
    Serial.println("Tabulka s nazvem: .");
    Serial.println( blaznivyNapad[jmeno]);
    Serial.println("byla zalozena");
  }
  else
    Serial.println("Connection failed.");
  conn.close();
}
//**********************************************************************************************************************************
//necekane oznameni
//**********************************************************************************************************************************
void zmenaRvi() {
  char pohl[8];
  sprintf(pohl, "%d\n", pohyb);
  char svetlo[15];
  sprintf(svetlo, "%d\n", lux);
  char array[20];
  sprintf(array, "%d:%d\n", pohyb, lux);

  for (int i = 0; i < pocetKlientu; i++) {
    switch (coPoMeKdoChce[i]) {
      case 1: {
          clients[i]->write(pohl);
          break;
        }
      case 2: {
          clients[i]->write(svetlo);
          break;
        }
      case 3: {
          clients[i]->write(array);
          break;
        }
    }

  }
}
//**********************************************************************************************************************************
//prikazy od clientely
//**********************************************************************************************************************************
void praceSklienty() {
  WiFiClient client = wifiServer.available();
  if (client) {
    Serial.println("new client");
    // Find the first unused space
    if (pocetKlientu < Maximu_clientu) {
      clients[pocetKlientu] = new WiFiClient(client);
      pocetKlientu++;
    }
  }
  for (byte i = 0 ; i < pocetKlientu ; ++i) {
    if (clients[i]->connected()) {
      if (clients[i]->available() > 0) {
        byte n = clients[i]->available();
        for (byte ij = 0; ij < n; ij++) {
          inputs[i][ij] = clients[i]->read();
          //Serial.printf("%d/%d: znak %c a jeho cislo %d \n", n, ij, inputs[i][ij], int(inputs[i][ij]));
        }
        if (inputs[i][n - 2] == 13 || inputs[i][n - 2] == 10) inputs[i][n - 2] = '\0';
        if (inputs[i][n - 1] == 13 || inputs[i][n - 1] == 10) inputs[i][n - 1] = '\0';
        Serial.println(inputs[i]);  //čte přicházející pakety
        zpracujPozadavek(inputs[i], i);
      }
    }
    else {
      pocetKlientu--;
      clients[i] = clients[pocetKlientu];
      coPoMeKdoChce[i] = coPoMeKdoChce[pocetKlientu];
      Serial.println("klient smazan");
      i--;
    }
  }
}
void zpracujPozadavek(char pole[mixim_delka_lajny], byte kteryKokot) {
  char array[20];
  if (strstr(pole, "p:s") ) {
    Serial.print("pohyb: ");
    Serial.println(pohyb);
    Serial.print("intenzita světla[lux]: ");
    Serial.println(lux);
    sprintf(array, "%d:%d \n", pohyb, lux);
    clients[kteryKokot]->write(array);
    Serial.println(array);
    coPoMeKdoChce[kteryKokot] = 3;
  }
  else if (strstr(pole, "p") ) {
    Serial.print("pohyb: ");
    Serial.println(pohyb);
    sprintf(array, "%d \n", pohyb);
    clients[kteryKokot]->write(array);
    Serial.println(array);
    coPoMeKdoChce[kteryKokot] = 1;
  }
  else if (strstr(pole, "s") ) {
    Serial.print("intenzita světla[lux]: ");
    Serial.println(lux);
    sprintf(array, "%d \n", lux);
    clients[kteryKokot]->write(array);
    Serial.println(array);
    coPoMeKdoChce[kteryKokot] = 2;
  }

}
//**********************************************************************************************************************************
//prvni kofigurace pro pripojeni
//**********************************************************************************************************************************
void konfigurace_init() {
  Serial.begin(115200);
  //nactiUdaje()
  //pripojSa();
  if (nactiUdaje() == 1) {
    pripojSa();
    wifiServer.begin();//začni server

  }
  pinMode(LED, OUTPUT);
  //neco jako timer interrupt ve vetřinách
  blinker.attach(1, konfigurace);
  luxSenzor.begin();
  lux = luxSenzor.readLightLevel();
  pohyb = false;
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, CHANGE);
}
void vycistiEepromku() {
  EEPROM.begin(velikostEpromky);
  // vycisti eepromku
  for (byte i = 0; i < velikostEpromky; i++) {
    EEPROM.write(i, '\0');
  }
  EEPROM.end();
}
byte nactiUdaje() {
  EEPROM.begin(velikostEpromky);
  byte posuv = 0;
  byte bylo = 0;
  for (byte poradi = 0; poradi < 6; poradi++) {
    byte n = EEPROM.read(posuv++);
    if (n > 0) {
      bylo = 1;
      for (byte i = 0; i < n; i++) {
        blaznivyNapad[poradi][i] = EEPROM.read(i + posuv);
      }
      udaje[poradi] = blaznivyNapad[poradi];
      posuv += 32;
      Serial.println(udaje[poradi]);
    }

  }
  for (byte i = 0; i < 4; i++) {
    dbAdresa[i] = EEPROM.read(i + posuv);
    Serial.print(dbAdresa[i]);
  }
  Serial.println();
  posuv += 4;
  port = 0;
  int mocnina = 1;
  for (byte i = 0; i < 4; i++) {
    port += EEPROM.read(i + posuv) * mocnina;
    mocnina *= 256;
  }
  Serial.println(port);
  EEPROM.end();
  IPAddress server_addr(dbAdresa[0], dbAdresa[1], dbAdresa[2], dbAdresa[3]);
  return bylo;
}
void prosteToPrecti() {
  EEPROM.begin(velikostEpromky);
  for (byte i = 0; i < velikostEpromky; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(char(EEPROM.read(i)));
  }
  EEPROM.end();
}
void scanujSite() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("Scan start ... ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" network(s) found");
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();

}
bool pripojSa() {
  Serial.println(udaje[1]);
  Serial.println(udaje[2]);
  WiFi.begin(udaje[1], udaje[2]);
  byte pocitadlo = 10;
  while (WiFi.status() != WL_CONNECTED && pocitadlo > 0) {
    delay(1000);
    Serial.println("Connecting..");
    pocitadlo--;
  }

  Serial.print("Connected to WiFi. IP:");
  Serial.println(WiFi.localIP());
  return WiFi.status();

}
void zapisDataDoEpromky(byte kdeJsmeVzapisu, byte n) {
  EEPROM.begin(velikostEpromky);
  if (kdeJsmeVzapisu <= 6) {
    byte posuv = (kdeJsmeVzapisu - 1) * 33;
    //zapise na prvni pozici velikost pole jak bude velke zapis jmena zarizeni
    EEPROM.write(posuv++, n);
    for (byte i = 0; i < n; i++) {
      EEPROM.write(i + posuv, blaznivyNapad[kdeJsmeVzapisu - 1][i]);
    }
  }
  else  {
    byte posuv = velikostEpromky - 8;
    for (byte i = 0; i < n; i++) {
      EEPROM.write(i + posuv, dbAdresa[i]);
    }
  }
  EEPROM.end();
}
void zapisDataDoEpromky(int portak) {
  //Serial.println(portak);
  EEPROM.begin(velikostEpromky);
  byte posuv = velikostEpromky - 4;
  for (byte i = 0; i < 4 && portak > 0; i++) {
    EEPROM.write(i + posuv, portak % 256);
    portak = portak / 256;
  }
  EEPROM.end();
}
void konfigurace()
{
  if (Serial.available() > 0) //Checks is there any data in buffer
  {
    byte n = Serial.available();
    char pole[n];
    for (byte i = 0; i < n; i++) {
      pole[i] = Serial.read();
    }
    pole[n - 1] = '\0';
    Serial.println(pole);  //Read serial data byte and send back to serial monitor
    switch (stavKomunikace) {
      case 1: {
          vycistiEepromku();
          for (byte i = 0; i <= n; i++)blaznivyNapad[jmeno][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("zadej jmeno wifi site");
          oscanuj = 1;
          break;
        }
      case 2: {
          for (byte i = 0; i < n; i++)blaznivyNapad[wifi][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace , n);
          stavKomunikace++;
          Serial.println("zadej heslo wifi site");
          break;
        }
      case 3: {
          for (byte i = 0; i < n; i++)blaznivyNapad[wifiHeslo][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace ++;
          Serial.println("ted zzadej jmeno blaznivyNapad [jmenoDB]");
          //nactiUdaje();
          //pripojSa();
          break;
        }
      case 4: {
          for (byte i = 0; i <= n; i++)blaznivyNapad[jmenoDB][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("jmeno pro prihlaseni do db");
          break;
        }
      case 5: {
          for (byte i = 0; i < n; i++)blaznivyNapad[PjmenoDB][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("heslo pro prihlaseni do db");
          break;
        }
      case 6: {
          for (byte i = 0; i < n; i++)blaznivyNapad[PhesloDB][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("zadej ip adresu serveru databaze ve tvaru 255.255.255.25:8080");
          break;
        }
      case 7: {
          port = 0;
          byte pocekOdectu = 0;
          int portMocnina = 1;
          for (byte i = n - 2; pole[i] != ':'; i--) {
            port += (pole[i] - 48) * portMocnina;
            //Serial.printf("takze tohle je %d jedno cislo krát tohla mocnina %d + konecny port %d\n",pole[i]-48,portMocnina,port);
            pocekOdectu++;
            portMocnina *= 10;
          }
          byte mocnina = 1;
          for (byte i = 0; i < 4; i++)dbAdresa[i] = 0;
          byte ij = 3;
          for (int i = n - 2 - pocekOdectu; i >= 0; i--) {
            if (pole[i] == '.') {
              ij--;
              mocnina = 1;
            }
            else if (pole[i] - 48 >= 0 && pole[i] - 48 <= 9) {
              dbAdresa[ij] += (pole[i] - 48) * mocnina;
              //Serial.printf("icko je:%d jeste ij: %d takze tohle ip je %d jedno cislo krát tohla mocnina %d + konecna adresa %d\n",i,ij,pole[i]-48,mocnina,dbAdresa[ij]);
              mocnina *= 10;
            }

          }
          n = 4;
          zapisDataDoEpromky(stavKomunikace, n);
          zapisDataDoEpromky(port);
          stavKomunikace = 0;
          Serial.println("konfigurace byla dokončena chces li ji zmenit zadej ahoj");
          setup();
          break;
        }
    }
    if (strcmp(pole, "ahoj") == 0) {
      Serial.println("Co je moje jméno?");
      stavKomunikace = 1;
    }
    if (strcmp(pole, "zobraz") == 0) {
      Serial.println(nactiUdaje());
    }
    if (strcmp(pole, "proste to precti") == 0) {
      prosteToPrecti();
    }
    if (strcmp(pole, "vycisti to") == 0) {
      vycistiEepromku();
    }
    if (strcmp(pole, "nacpi to tam na pevno") == 0) {
      const char* neco = "2.4GHz_nech_me_byt";
      const char* peco = "01189998819991197253";
      udaje[1] = neco;
      udaje[2] = peco;
    }
    if (strcmp(pole, "pripoj sa") == 0) {
      pripojSa();
      wifiServer.begin();//začni server
    }

  }
  //Invert Current State of LED
}
