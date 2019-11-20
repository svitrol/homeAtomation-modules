#define jmeno 0
#define wifi 1
#define wifiHeslo 2
#define jmenoDB 3
#define PjmenoDB 4
#define PhesloDB 5
#define PocetRelecek 8
#define Maximu_clientu 10
#define mixim_delka_lajny 50
#define maxim_delkaPoleNaPodminky 1000

#include <cas.h>
#include <NTPClient.h>
#include "ESP8266WiFi.h"
#include <WiFiUdp.h>
#include "EEPROM.h"
#include <Ticker.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>


#define LED 2

WiFiServer wifiServer(8888);
#define pocetUdaju 6
const char* udaje[pocetUdaju];
char blaznivyNapad [pocetUdaju][32];
byte stavKomunikace = 0;
byte velikostEpromky = 206;//32 jmeno zarizeni 32 jmeno wifi 32 heslo wifi + 3 pokazde velikost pred obsahem promene  plus novinka 32 jmeno db 32 jmeno 32 heslo +4 ip serveru + 4 na port takze celk velikost 6*33 +4+4
const char* necekanaZprava = "budik";
byte necekane = 0;
volatile byte relatka[PocetRelecek] = {0, 0, 0, 0, 0, 0, 0, 0};
//volatile unsigned int odpocet[PocetRelecek];

WiFiClient nakladac;
MySQL_Connection conn((Client *)&nakladac);
byte  dbAdresa[4] = {0, 0, 0, 0};
IPAddress server_addr(88, 86, 120, 229); // IP of the MySQL *server* here
int port = 3310;
//pro zalozeni tabulky
const char* zlozni = "CREATE TABLE IF NOT EXISTS `%s`.`%s` ( `id` INT NOT NULL AUTO_INCREMENT ,  `R1` BOOLEAN NOT NULL ,  `R2` BOOLEAN NOT NULL ,  `R3` BOOLEAN NOT NULL ,  `R4` BOOLEAN NOT NULL ,  `R5` BOOLEAN NOT NULL ,  `R6` BOOLEAN NOT NULL ,  `R7` BOOLEAN NOT NULL ,  `R8` BOOLEAN NOT NULL ,  `casUpravy` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ,    PRIMARY KEY  (`id`)) ENGINE = MyISAM CHARSET=utf8;";
const char* prvniRadek = " INSERT  IGNORE INTO `%s`.`%s` (`id`, `R1`, `R2`, `R3`, `R4`, `R5`, `R6`, `R7`, `R8`, `casUpravy`) VALUES (1, '0', '0', '0', '0', '0', '0', '0', '0', CURRENT_TIMESTAMP);";
//"CREATE TABLE IF NOT EXISTS `%s`.`%s` ( `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT ,  `cas` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ,  `vlhkost` FLOAT NOT NULL ,  `teplota` FLOAT NOT NULL ,  `zprava` CHAR(40) NOT NULL ,    PRIMARY KEY  (`id`)) ENGINE = MyISAM;";
// Sample query
const char* Update_Data = "UPDATE `%s`.`%s` SET `R1`=%d, `R2`=%d, `R3`=%d, `R4`=%d, `R5`=%d, `R6`=%d, `R7`=%d, `R8`=%d, `casUpravy`=CURRENT_TIMESTAMP WHERE `id`=1;";
const char* VYber_Data = "SELECT * FROM `%s`.`%s` WHERE `id`=1;";
bool oscanuj = 0;
bool uprava = 0;
bool cekamNaOdpoved = 0;
bool zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich = false;
bool poslirelecka = 0;
byte kromkdoToPoslal = 255;
//volatile bool posilamTabulku=false;
//volatile byte coPosilamTabulku=0;
char posledniZparava[10];

Ticker blinker;
cas systemak();
//volatile uint8_t sekundy, minuty, hodiny;



WiFiClient *clients[Maximu_clientu] = { NULL };
char inputs[Maximu_clientu][mixim_delka_lajny] = { 0 };
	
WiFiClient senzory[PocetRelecek];
byte pocetAktivnichCteni = 0;
float cekovanyHodnyto[PocetRelecek][2];
char vysledek[maxim_delkaPoleNaPodminky];
uint16_t mentalniVelikostpodminke=0;
const char* prikazy[8];
const char* podminky[20];
int pocetPodminek = 0;

unsigned int casek, ctiSeriak,ctiUchazece,cticas;
int pocetKlientu = 0;
byte bylJsemVdB = 0;


const long UTCposun = 3600;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTCposun);

void setup() {

  konfigurace_init();
  //neco jako timer interrupt ve vetřinách
  blinker.attach(1, pocitadlo_hodin);

}



void loop() {
  praceSklienty();
  if (uprava == 1) {
    updateDB();
    uprava = 0;
  }
  if (millis() - casek >= 1000) {
    //Serial.println("#Kontrola");
    zmeniloSeDb();
    casek = millis();
  }
  if (millis() - ctiSeriak >= 500) {
    konfigurace();
    ctiSeriak = millis();
  }
  if (millis() - ctiUchazece >= 1000) {
	  rutiny();
	  ctiUchazece = millis();
  }
  if (necekane) {
    tohleNecekali();
  }
  if (oscanuj) {
    oscanuj = 0;
    scanujSite();
  }
  if (zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich) {
    zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich = false;
    zmanaRvi();
  }
  if (poslirelecka) {
    hodMiPsledniDoNul();
    byte zapunty = hodMiRelatkaDoCisla();
    if (zapunty == 0) {
      sprintf(posledniZparava, "RX:0\n");
    }
    else {
      sprintf(posledniZparava, "P%c\n", hodMiRelatkaDoCisla());
    }
    Serial.print(posledniZparava);
    cekamNaOdpoved = true;
    poslirelecka = 0;
    kromkdoToPoslal = 255;
  }


}
//**********************************************************************************************************************************
//zjistovani z maluho senzoru
//**********************************************************************************************************************************
//tvar zadani S192.168.1.100:8888[v:t];192.168.1.114:8888[p];192.168.1.117:8888[s]{v01>20?R1:0-R1:1;t00>0?R1:1-R1:0}
//tvar na cas St{0:0:0 08.11.2019?R1:0-R1:1;v/t/v:t>0?R1:1-R1:0;}
//tvar na cas St{0:0:0-02:0:0?R1:0-R1:1;v/t/v:t>0?R1:1-R1:0;}
void rutiny(){
	bool pribylNovaHodnota;
	for (byte i = 0 ; i < pocetAktivnichCteni ; ++i) {
		if (senzory[i].connected()) {
			if (senzory[i].available() > 0) {
				byte n = senzory[i].available();
				char hodnoty[12];
				byte poziceNecisla=255;
				for (byte ij = 0; ij < n; ij++) {
					hodnoty[ij] = senzory[i].read();
					if(hodnoty[ij]<'0'||hodnoty[ij]>9){
						poziceNecisla=ij;
					}
				}
				if (inputs[i][n - 2] == 13 || inputs[i][n - 2] == 10) inputs[i][n - 2] = '\0';
				if (inputs[i][n - 1] == 13 || inputs[i][n - 1] == 10) inputs[i][n - 1] = '\0';
				if(poziceNecisla==255){
					cekovanyHodnyto[i][0]=atof(hodnoty);
				}
				else{
					hodnoty[poziceNecisla]='\0';
					const char* ukazNaPrvni=&hodnoty;
					const char* ukazNaDruhy=&hodnoty[poziceNecisla+1];
					cekovanyHodnyto[i][0]=atof(ukazNaPrvni);
					cekovanyHodnyto[i][0]=atof(ukazNaDruhy);					
				}
				pribylNovaHodnota=true;
			}
		}
	}
	if(pribylNovaHodnota){
		projdiSiPodminky();
	}
}
void projdiSiPodminky(){
	for (int i = 0; i < pocetPodminek; i++) {
		podminka(podminky[i]);
	}
}
void podminka(const char* odkaz) {
	char podminka[3];
	uint8_t icko = 0;
	char skoncil;
	char prvniCastPodminky[20];
	char druhaCastPodminky[20];
	uint8_t pocatek;

	for (pocatek = icko; odkaz[icko] != '<'&& odkaz[icko] != '>' && odkaz[icko] != '=' && odkaz[icko] != '!'; icko++) {
		prvniCastPodminky[icko - pocatek] = odkaz[icko];
	}
	prvniCastPodminky[icko - pocatek] = '\0';
	podminka[0] = odkaz[icko];
	if (odkaz[++icko] == '=') {
		podminka[1] = odkaz[icko++];
		podminka[2] = '\0';
	}
	else {
		podminka[1] = '\0';
	}
	for (pocatek = icko; odkaz[icko] != '?' && odkaz[icko] != '&' && odkaz[icko] != '|'; icko++) {
		druhaCastPodminky[icko - pocatek] = odkaz[icko];
	}
	druhaCastPodminky[icko - pocatek] = '\0';
	skoncil = odkaz[icko];
	//icko++;
	bool a = false;
	if (prvniCastPodminky[0] == 't' || prvniCastPodminky[0] == 'c') {
		cas casak1 = preberSiCasaky(prvniCastPodminky);
		cas casak2 = preberSiCasaky(druhaCastPodminky);
		a = rozhodovacka(podminka, casak1, casak2);
		//printf(" %d:%d:%d %d.%d.%d %s %d:%d:%d %d.%d.%d  dopadlo: %d\n",  casak1.sekundy, casak1.minuty, casak1.hodiny, casak1.dny, casak1.mesice, casak1.roky, podminka, casak2.sekundy, casak2.minuty, casak2.hodiny, casak2.dny, casak2.mesice, casak2.roky,a);
	}
	else {
		float cislo1 = preberSiKonstanty(prvniCastPodminky);
		float cislo2 = preberSiKonstanty(druhaCastPodminky);
		a = rozhodovacka(podminka, cislo1, cislo2);
		//printf("%f %s %f  dopadlo: %d\n",  cislo1, podminka, cislo2,a);
	}
	postarejSeOTo(icko, odkaz, skoncil, a);
}
void podminka(const char* odkaz, bool jakyBylStav) {
	char podminka[3];
	uint8_t icko = 0;
	char zacatek = odkaz[icko++];
	char skoncil;
	char prvniCastPodminky[20];
	char druhaCastPodminky[20];
	uint8_t pocatek;

	for (pocatek = icko; odkaz[icko] != '<'&& odkaz[icko] != '>' && odkaz[icko] != '=' && odkaz[icko] != '!'; icko++) {
		prvniCastPodminky[icko - pocatek] = odkaz[icko];
	}
	prvniCastPodminky[icko - pocatek] = '\0';
	podminka[0] = odkaz[icko];
	if (odkaz[++icko] == '=') {
		podminka[1] = odkaz[icko++];
		podminka[2] = '\0';
	}
	else {
		podminka[1] = '\0';
	}
	for (pocatek = icko; odkaz[icko] != '?' && odkaz[icko] != '&' && odkaz[icko] != '|'; icko++) {
		druhaCastPodminky[icko - pocatek] = odkaz[icko];
	}
	druhaCastPodminky[icko - pocatek] = '\0';
	skoncil = odkaz[icko];
	//icko++;

	bool a = false;
	if (prvniCastPodminky[0] == 't' || prvniCastPodminky[0] == 'c') {
		cas casak1 = preberSiCasaky(prvniCastPodminky);
		cas casak2 = preberSiCasaky(druhaCastPodminky);
		a = rozhodovacka(jakyBylStav, zacatek, podminka, casak1, casak2);
		//printf("%d%c %d:%d:%d %d.%d.%d %s %d:%d:%d %d.%d.%d dopadlo: %d\n", jakyBylStav, zacatek, casak1.sekundy, casak1.minuty, casak1.hodiny, casak1.dny, casak1.mesice, casak1.roky, podminka, casak2.sekundy, casak2.minuty, casak2.hodiny, casak2.dny, casak2.mesice, casak2.roky, a);
	}
	else {
		float cislo1 = preberSiKonstanty(prvniCastPodminky);
		float cislo2 = preberSiKonstanty(druhaCastPodminky);
		a = rozhodovacka(jakyBylStav, zacatek, podminka, cislo1, cislo2);
		//printf("%d%c %f %s %f dopadlo: %d\n", jakyBylStav, zacatek, cislo1, podminka, cislo2, a);
	}
	postarejSeOTo(icko, odkaz, skoncil, a);
}

float preberSiKonstanty( const char* cisloProPrevod) {
	float vysledek = 0;
	if (cisloProPrevod[0] < '0' || cisloProPrevod[0]>'9') {
		if (cisloProPrevod[0] == 'R') {
			vysledek = relatka[cisloProPrevod[1] - 48 - 1];
		}
		else {
			vysledek = cekovanyHodnyto[cisloProPrevod[1] - 48][cisloProPrevod[2] - 48];
		}
	}
	else {
		vysledek = atof(cisloProPrevod);
	}
	return vysledek;
}
cas preberSiCasaky(char* cisloProPrevod) {
	cas vysledek;
	if (cisloProPrevod[0] == 'c') {
		const char* poleslav[6];;
		uint8_t pocet=1;
		poleslav[0] = &cisloProPrevod[1];
		for (uint8_t i = 0; cisloProPrevod[i] != '\0';i++) {
			if (cisloProPrevod[i] == '-' || cisloProPrevod[i] == ':' || cisloProPrevod[i] == '.') {
				char* zmana = &cisloProPrevod[i++];
				*zmana= '\0';

				poleslav[pocet++]=&cisloProPrevod[i];
			}
		}
		if (pocet == 3) {
			vysledek.sekundy = atoi(poleslav[0]);
			vysledek.minuty = atoi(poleslav[1]);
			vysledek.hodiny = atoi(poleslav[2]);
		}
		else {

			vysledek.sekundy = atoi(poleslav[0]);
			vysledek.minuty = atoi(poleslav[1]);
			vysledek.hodiny = atoi(poleslav[2]);
			vysledek.dny = atoi(poleslav[3]);
			vysledek.mesice = atoi(poleslav[4]);
			vysledek.roky = atoi(poleslav[5]);
		}
	}
	else {
		vysledek = systemak;
	}
	return vysledek;
}
void vykonajFejnovyPrikaz(const char* prikaz){}
void zpracujVysledek(){
	if (vysledek[0] == 'S') {
		uint16_t icko;
		if (vysledek[1] != 't') {
			icko = 0;
			uint8_t pocetPrikaziva = 0;
			while (vysledek[icko] != '{') {
				char ipina[16];
				int portakovaporta = 0;
				char porta[6];
				char prikaz[6];
				uint16_t pocatek;

				for (pocatek = ++icko; vysledek[icko] != ':'; icko++) {
					ipina[icko - pocatek] = vysledek[icko];
				}
				ipina[icko - pocatek] = '\0';
				vysledek[icko] = '\0';
				for (pocatek = ++icko; vysledek[icko] != '['; icko++) {
					porta[icko - pocatek] = vysledek[icko];
				}
				porta[icko - pocatek] = '\0';
				prikazy[pocetPrikaziva++] = &vysledek[icko + 1];
				vysledek[icko] = '\0';
				for (pocatek = ++icko; vysledek[icko] != ']'; icko++) {
					prikaz[icko - pocatek] = vysledek[icko];
				}
				prikaz[icko - pocatek] = '\0';
				vysledek[icko] = '\0';
				//vysledek[++icko] = '\0';
				icko++;
				if (senzory[pocetAktivnichCteni].connect(ipina, atoi(porta))) {
					pocetAktivnichCteni++;
					senzory[pocetAktivnichCteni].printf("%s\n",prikaz);
				}
				else{
					Serial.print("Ten tvuj senzor nejak nejede\n");
				}
				if(vysledek[icko]==';')vysledek[icko] = '\0';
			}
		}
		else {
			icko = 2;
		}
		podminky[pocetPodminek++] = &vysledek[icko + 1];
		for (uint8_t i = icko; i < 200; i++) {
			if (vysledek[i] == ';') {
				vysledek[i] = '\0';
				podminky[pocetPodminek++] = &vysledek[i + 1];
			}
		}
		/*//projed podminky
		for (uint8_t i = 0; i < pocetPodminek; i++) {
			podminka(podminky[i]);
		}*/	

	}
}
void preberSiTo(char pole[mixim_delka_lajny],byte kteryKokot){
	//char pole[200];
	const char* prikazy[8];
	//S192.168.1.100:8888[v:t];192.168.1.114:8888[p];192.168.1.117:8888[s]{v>2?R1:0-R1:1;t>0?R1:1-R1:0}
	//S192.168.1.100:8888[v:t];192.168.1.114:8888[p];192.168.1.117:8888[s]{v01>2&v00>3&v01<3?R1:0-R1:1;t>0?R1:1-R1:0}
	//S192.168.1.100:8888[v:t];192.168.1.114:8888[p];192.168.1.117:8888[s]{v00>20?R1:0-R1:1;v10>0?R1:1-R1:0;c00:00:00-1.12.2019=t?R1:0:x00:00:00-R1:1:1}
	//St{c00:00:00-1.12.2019=t?R1:0:x00:00:00-R1:1:1}
	if (pole[0] == 'S') {
		if (pole[1] != 'p') {
			bool bylaUkonceni=false;
			mentalniVelikostpodminke=0;
			while(pole[mentalniVelikostpodminke]!='\0'){
				vysledek[mentalniVelikostpodminke]=pole[mentalniVelikostpodminke];
				if(vysledek[mentalniVelikostpodminke]=='}')bylaUkonceni=true;
				mentalniVelikostpodminke++;
			}
			if(bylaUkonceni){
				zpracujVysledek();
			}
		}
		else{
			uint16_t pocatek=mentalniVelikostpodminke;
			bool bylaUkonceni=false;
			while(pole[mentalniVelikostpodminke]!='\0'){
				vysledek[mentalniVelikostpodminke]=pole[mentalniVelikostpodminke-pocatek];
				if(vysledek[mentalniVelikostpodminke]=='}')bylaUkonceni=true;
					mentalniVelikostpodminke++;
			}
			if(bylaUkonceni){
				zpracujVysledek();
			}
		}
	}
}
void postarejSeOTo(uint8_t icko, const char* odkaz, char skoncil, bool jakToDopadlo) {
	uint8_t pocatek;
	if (jakToDopadlo) {
		if (skoncil != '?') {
			podminka(&odkaz[icko], true);
		}
		else {
			char prikaz[50];
			for (pocatek = ++icko; odkaz[icko] != '-'; icko++) {
				prikaz[icko-pocatek]=odkaz[icko];
				vykonajFejnovyPrikaz(prikaz);
			}
		}
	}
	else if (skoncil != '?') {
		podminka(&odkaz[icko], false);
	}
	else {
		char prikaz[50];
		for (pocatek = icko; odkaz[icko] != '-'; icko++);
		for (pocatek = ++icko; odkaz[icko] != '\0' && odkaz[icko] != '}'; icko++) {
		prikaz[icko-pocatek]=odkaz[icko];
	}
	vykonajFejnovyPrikaz(prikaz);
}}
template<typename T>
bool rozhodovacka(const char* podminka,T prvniCastPodminky,T druhaCastPodminky) {
	bool a = false;
	switch (podminka[0]) {
		case '<': {
			if (podminka[1] == '=') {
				a = prvniCastPodminky <= druhaCastPodminky;
			}
			else {
				a = prvniCastPodminky < druhaCastPodminky;
			}
			break;
		}
		case '>': {
			if (podminka[1] == '=') {
				a = prvniCastPodminky >= druhaCastPodminky;
			}
			else {
				a = prvniCastPodminky > druhaCastPodminky;
			}
			break;

		}
		case '=': {
			a = prvniCastPodminky == druhaCastPodminky;
			break;

		}
		case '!': {
			a = prvniCastPodminky != druhaCastPodminky;

			break;
		}
	}
	return a;
}
template<typename T>
bool rozhodovacka(bool jakyBylStav,char zacatek, const char* podminka, T prvniCastPodminky, T druhaCastPodminky) {
	bool a = false;
	if (zacatek == '|') {

		switch (podminka[0]) {
			case '<': {
				if (podminka[1] == '=') {
					a = jakyBylStav || prvniCastPodminky <= druhaCastPodminky;
				}
				else {
					a = jakyBylStav || prvniCastPodminky < druhaCastPodminky;
				}
				break;
			}
			case '>': {
				if (podminka[1] == '=') {
					a = jakyBylStav || prvniCastPodminky >= druhaCastPodminky;
				}
				else {
					a = jakyBylStav || prvniCastPodminky > druhaCastPodminky;
				}
				break;

			}
			case '=': {
				a = jakyBylStav || prvniCastPodminky == druhaCastPodminky;
				break;

			}
			case '!': {
				a = jakyBylStav || prvniCastPodminky != druhaCastPodminky;

				break;
			}
		}
	}
	else {
		switch (podminka[0]) {
			case '<': {
				if (podminka[1] == '=') {
					a = jakyBylStav && prvniCastPodminky <= druhaCastPodminky;
				}
				else {
					a = jakyBylStav && prvniCastPodminky < druhaCastPodminky;
				}
				break;
			}
			case '>': {
				if (podminka[1] == '=') {
					a = jakyBylStav && prvniCastPodminky >= druhaCastPodminky;
				}
				else {
					a = jakyBylStav && prvniCastPodminky > druhaCastPodminky;
				}
				break;

			}
			case '=': {
				a = jakyBylStav && prvniCastPodminky == druhaCastPodminky;
				break;

			}
			case '!': {
				a = jakyBylStav && prvniCastPodminky != druhaCastPodminky;

				break;
			}
		}
	}
	return a;
}

//**********************************************************************************************************************************
//zjistovani casu
//**********************************************************************************************************************************

void pocitadlo_hodin() {
	
  if (++systemak.sekundy >= 60) {
    systemak.sekundy = 0;
    if (++systemak.minuty >= 60) {
      systemak.minuty = 0;
      if (++systemak.hodiny >= 24) {
        systemak.hodiny = 0;
      }
    }
	if (WiFi.status() == WL_CONNECTED) {
		timeClient.update();
		systemak.hodiny = timeClient.getHours();
		systemak.minuty = timeClient.getMinutes();
		systemak.sekundy = timeClient.getSeconds();
		Serial.printf("#cas je updatnuty: %d:%d:%d\n",systemak.hodiny,systemak.minuty,systemak.sekundy);
	}
  }
}
//**********************************************************************************************************************************
//zjistovani z maluho arduina
//**********************************************************************************************************************************
void provokace(char pole[], byte n) {
  if (cekamNaOdpoved)	{
    if (pole[0] == 'y') {
      cekamNaOdpoved = 0;
      //posliToCely();
    }
    else {
      Serial.println(posledniZparava);
    }
  }
  if (pole[0] == 'P') {
    prevedNaPiny(pole[1]);
    Serial.printf("yx\n");
    uprava = 1;
    zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich = true;
  }
  //Serial.println(pole);
  else if (pole[0] == 'R') {
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
          if (pole[3] == '1') {
            akce = 1;
            //Serial.printf("ano zapnu ti ho %s",pole);
          }
          if (akce != relatka[pole[1] - 48 - 1]) {
            relatka[pole[1] - 48 - 1] = akce;
            uprava = 1;
            Serial.printf("y%d", akce);
            zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich = true;
          }


          //Serial.println(pole);

          break;
        }
      case 40: {
          if (pole[3] == '1') {
            for (byte pohoda = 0; pohoda < 8; pohoda++) {
              relatka[pohoda] = 1;
            }
            uprava = 1;
            Serial.printf("yx\n");
            zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich = true;

          }
          if (pole[3] == '0') {
            for (byte pohoda = 0; pohoda < 8; pohoda++) {
              relatka[pohoda] = 0;
            }
            uprava = 1;
            Serial.printf("yx\n");
            zmanaCoBySeMelaUkazatNaMobilechaDalsichZarizenich = true;
          }
          if (pole[3] == '?')poslirelecka = 1;
          break;
        }
      default: {
          Serial.println("#to je divny!");
          break;
        }
    }

  }
}


void prevedNaPiny(byte cislo) {
  for (byte i = 0; i < 8; i++) {
    relatka[i] = 0b00000001 & cislo;
    cislo = cislo >> 1;
  }

}

//**********************************************************************************************************************************
//zjistovani z db
//**********************************************************************************************************************************
void zmeniloSeDb() {
  if (bylJsemVdB == 1) {
    if (conn.connected()) {
      ctiDb();

    }
    else if (conn.connect(server_addr, port, blaznivyNapad [PjmenoDB], blaznivyNapad [PhesloDB])) {
      //delay(1000);
      ctiDb();

    }
    else {
      Serial.println("# Connection failed.");
    }
  }
}
void ctiDb() {
  row_values *row = NULL;
  int stavy[8];
  // Initiate the query class instance
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  // Execute the query
  char query[110];
  sprintf(query, VYber_Data,  blaznivyNapad [jmenoDB], blaznivyNapad[jmeno]);
  //Serial.printf("# %s",query);
  cur_mem->execute(query);
  // Fetch the columns (required) but we don't use them.
  column_names *columns = cur_mem->get_columns();

  // Read the row (we are only expecting the one)
  bool zmeniloSe = false;
  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      for (byte i = 0; i < 8; i++) {
        stavy[i] = atoi(row->values[i + 1]);
        if (stavy[i] != relatka[i]) {
          relatka[i] = stavy[i];
          zmeniloSe = true;
        }
      }
    }
  } while (row != NULL);
  if (zmeniloSe) {
    poslirelecka = 1;
  }
  // Deleting the cursor also frees up memory used
  delete cur_mem;

  // Show the result
  //Serial.printf("#R1: %d,R2: %d,R3: %d,R4: %d,R5: %d,R6: %d,R7: %d,R8: %d\n",stavy[0],stavy[1],stavy[2],stavy[3],stavy[4],stavy[5],stavy[6],stavy[7]);
  // Serial.printf("#R1: %d,R2: %d,R3: %d,R4: %d,R5: %d,R6: %d,R7: %d,R8: %d\n",relatka[0],relatka[1],relatka[2],relatka[3],relatka[4],relatka[5],relatka[6],relatka[7]);
}


//**********************************************************************************************************************************
//ulozeni do db
//**********************************************************************************************************************************
void updateDB() {
  if (bylJsemVdB == 1) {
    if (conn.connected()) {
      updatniTo();
    }
    else if (conn.connect(server_addr, port, blaznivyNapad [PjmenoDB], blaznivyNapad [PhesloDB])) {
      Serial.println("#Connecting...");
      //delay(1000);
      updatniTo();
    }
    else Serial.println("#Connection failed.");
  }
}

void updatniTo() {
  char query[255];
  // zprava co odešlu
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  // nactu hodnoty relatek

  sprintf(query, Update_Data,  blaznivyNapad [jmenoDB], blaznivyNapad[jmeno], relatka[0], relatka[1], relatka[2], relatka[3], relatka[4], relatka[5], relatka[6], relatka[7]);

  //Serial.printf("# %s",query);
  // zapise do db
  cur_mem->execute(query);
  delete cur_mem;
  Serial.println("#Data recorded.");
  //conn.close();
}
void zalozTabulku() {

  Serial.println("#Connecting...");
  if (conn.connect(server_addr, port, blaznivyNapad [PjmenoDB], blaznivyNapad [PhesloDB])) {
    delay(1000);
    char query[450];//676
    // zprava co odešlu
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    // nactu hodnoty
    sprintf(query, zlozni, blaznivyNapad [jmenoDB], blaznivyNapad[jmeno]);
    //Serial.printf("#%s\n",query);
    // zapise do db
    cur_mem->execute(query);
    sprintf(query, prvniRadek, blaznivyNapad [jmenoDB], blaznivyNapad[jmeno]);
    //Serial.printf("#%s\n",query);
    cur_mem->execute(query);

    delete cur_mem;
    Serial.printf("#Tabulka s nazvem: %s byla zalozena\n", blaznivyNapad[jmeno]);
    bylJsemVdB = 1;
  }
  else
    Serial.println("#Connection failed.");
  //conn.close();
}
//**********************************************************************************************************************************
//necekane oznameni
//**********************************************************************************************************************************
void tohleNecekali() {
  for (int i = 0; i < pocetKlientu; i++) {
    clients[i]->write(necekanaZprava);
  }
  necekane = 0;
}
void zmanaRvi() {
  char polickosCtyretiDvemaZnaky[42];
  sprintf(polickosCtyretiDvemaZnaky, "R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d\n", 1, relatka[0], 2, relatka[1], 3, relatka[2], 4, relatka[3], 5, relatka[4], 6, relatka[5], 7, relatka[6], 8, relatka[7]);
  for (int i = 0; i < pocetKlientu; i++) {
    if (i != kromkdoToPoslal) {
      clients[i]->write(polickosCtyretiDvemaZnaky);
    }

  }
}
//**********************************************************************************************************************************
//prikazy od clientely
//**********************************************************************************************************************************
void praceSklienty() {
  WiFiClient client = wifiServer.available();
  if (client) {
    Serial.println("#new client");
    // Find the first unused space
    clients[pocetKlientu] = new WiFiClient(client);
    pocetKlientu++;
    if (pocetKlientu >= Maximu_clientu)pocetKlientu = Maximu_clientu - 1;
  }
  for (byte i = 0 ; i < Maximu_clientu ; ++i) {
    if (NULL != clients[i]) {
      if (clients[i]->connected()) {
        if (clients[i]->available() > 0) {
          byte n = clients[i]->available();
          for (byte ij = 0; ij < n; ij++) {
            inputs[i][ij] = clients[i]->read();
            //Serial.write(inputs[i][ij]);
            //Serial.printf("%d/%d: znak %c a jeho cislo %d \n", n, ij, inputs[i][ij], int(inputs[i][ij]));
          }
          if (inputs[i][n - 2] == 13 || inputs[i][n - 2] == 10) inputs[i][n - 2] = '\0';
          if (inputs[i][n - 1] == 13 || inputs[i][n - 1] == 10) inputs[i][n - 1] = '\0';
          //Serial.println(inputs[i]);  //čte přicházející pakety
          zpracujPozadavek(inputs[i], i);
          hodMiPoleDoNul(inputs[i]);
        }
      }
      else {
        pocetKlientu--;
        clients[i] = clients[pocetKlientu];
        Serial.println("#klient smazan");
        i--;
        if (pocetKlientu < 0)pocetKlientu = 0;
      }

    }
  }
}
void hodMiPoleDoNul(char poleslav[]) {
  for (byte i = 0; i < mixim_delka_lajny; i++) {
    poleslav[i] = '\0';
  }
}
void hodMiPsledniDoNul() {
  for (byte i = 0; i < 10; i++) {
    posledniZparava[i] = '\0';
  }
}
byte hodMiRelatkaDoCisla() {
  byte cislo = 0;
  for (byte i = 0; i < 8; i++) {
    cislo += relatka[i] << i;
  }
  return cislo;

}
void zpracujPozadavek(char pole[mixim_delka_lajny], byte kteryKokot) {
  if (pole[0] == 'R') {
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
            uprava = 1;
            poslirelecka = 1;
            kromkdoToPoslal = kteryKokot;
          }
          clients[kteryKokot]->write("ok\n");
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
          if (bylaZmena) {
            clients[kteryKokot]->write("ok\n");
            uprava = 1;
            poslirelecka = 1;
            kromkdoToPoslal = kteryKokot;
          }
          if (pole[3] == '?') {
            char polickosCtyretiDvemaZnaky[42];
            sprintf(polickosCtyretiDvemaZnaky, "R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d;R%d:%d\n", 1, relatka[0], 2, relatka[1], 3, relatka[2], 4, relatka[3], 5, relatka[4], 6, relatka[5], 7, relatka[6], 8, relatka[7]);
            clients[kteryKokot]->write(polickosCtyretiDvemaZnaky);
          }
          break;
        }
      default: {
          clients[kteryKokot]->write("Co to po mě chceš?");
          break;

        }
    }
  }
  else if(pole[0] == 'S'){
	  preberSiTo(pole,kteryKokot);	  
  }
}
//tvar zadani S192.168.1.100:8888[v:t];192.168.1.114:8888[p];192.168.1.117:8888[s]{v/t/v:t>20?R1:0-R1:1;v/t/v:t>0?R1:1-R1:0;}
//tvar na cas St{0:0:0 08.11.2019?R1:0-R1:1;v/t/v:t>0?R1:1-R1:0;}
//tvar na cas St{0:0:0-02:0:0?R1:0-R1:1;v/t/v:t>0?R1:1-R1:0;}
//**********************************************************************************************************************************
//prvni kofigurace pro pripojeni
//**********************************************************************************************************************************
void konfigurace_init() {
  Serial.begin(9600);
  if (nactiUdaje() == 1) {
    pripojSa();
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.begin();
      wifiServer.begin();//začni server
    }

  }
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  casek = millis();
  ctiSeriak = millis() + 200;
  ctiUchazece=millis()+200;
  bylJsemVdB = 0;
  uprava = 0;
  if (WiFi.status() == WL_CONNECTED) {
    zalozTabulku();
    timeClient.update();
    systemak.hodiny = timeClient.getHours();
    systemak.minuty = timeClient.getMinutes();
    systemak.sekundy = timeClient.getSeconds();
  }
  else{
    systemak.hodiny = 0;
    systemak.minuty = 0;
    systemak.sekundy = 0;
  }
  mentalniVelikostpodminke=0;
  pocetKlientu = 0;
  pocetAktivnichCteni=0;
  
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
    }

  }
  for (byte i = 0; i < 4; i++) {
    dbAdresa[i] = EEPROM.read(i + posuv);
  }
  posuv += 4;
  port = 0;
  int mocnina = 1;
  for (byte i = 0; i < 4; i++) {
    port += EEPROM.read(i + posuv) * mocnina;
    mocnina *= 256;
  }
  EEPROM.end();
  IPAddress server_addr(dbAdresa[0], dbAdresa[1], dbAdresa[2], dbAdresa[3]);
  Serial.printf("# jmeno: %s\n# ssid: %s\n# password: %s\n# jmenoDB: %s\n# prihlasovaci jmeno do db: %s\n# heslo Db: %s\n# ip db: %d.%d.%d.%d:%d\n", blaznivyNapad[jmeno], blaznivyNapad[wifi], blaznivyNapad[wifiHeslo], blaznivyNapad[jmenoDB], blaznivyNapad[PjmenoDB], blaznivyNapad[PhesloDB], dbAdresa[0], dbAdresa[1], dbAdresa[2], dbAdresa[3], port);
  return bylo;
}
void prosteToPrecti() {
  EEPROM.begin(velikostEpromky);
  for (byte i = 0; i < velikostEpromky; i++) {
    Serial.printf("#%d. %c", i, char(EEPROM.read(i)));
  }
  EEPROM.end();
}
void scanujSite() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.print("#Scan start ... ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println("# network(s) found");
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();

}
void pripojSa() {
  Serial.printf("#%s\n %s\n", blaznivyNapad[wifi], blaznivyNapad[wifiHeslo]);
  WiFi.begin(udaje[1], udaje[2]);
  byte pocitadlo = 10;
  while (WiFi.status() != WL_CONNECTED && pocitadlo > 0) {
    delay(1000);
    Serial.println("#Connecting..");
    pocitadlo--;
  }

  Serial.print("# Connected to WiFi. IP:");
  Serial.println(WiFi.localIP());

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
    //Serial.printf("#%s\n",pole);  //pošli to zpátky
    switch (stavKomunikace) {
      case 1: {
          vycistiEepromku();
          for (byte i = 0; i <= n; i++)blaznivyNapad[jmeno][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("#zadej jmeno wifi site");
          oscanuj = 1;
          break;
        }
      case 2: {
          for (byte i = 0; i < n; i++)blaznivyNapad[wifi][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace , n);
          stavKomunikace++;
          Serial.println("#zadej heslo wifi site");
          break;
        }
      case 3: {
          for (byte i = 0; i < n; i++)blaznivyNapad[wifiHeslo][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace ++;
          Serial.println("#ted zzadej jmeno blaznivyNapad [jmenoDB]");
          //nactiUdaje();
          //pripojSa();
          break;
        }
      case 4: {
          for (byte i = 0; i <= n; i++)blaznivyNapad[jmenoDB][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("#jmeno pro prihlaseni do db");
          break;
        }
      case 5: {
          for (byte i = 0; i < n; i++)blaznivyNapad[PjmenoDB][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("#heslo pro prihlaseni do db");
          break;
        }
      case 6: {
          for (byte i = 0; i < n; i++)blaznivyNapad[PhesloDB][i] = pole[i];
          zapisDataDoEpromky(stavKomunikace, n);
          stavKomunikace++;
          Serial.println("#zadej ip adresu serveru databaze ve tvaru 255.255.255.25:8080");
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
          Serial.println("# konfigurace byla dokončena chces li ji zmenit zadej ahoj");
          setup();
          break;
        }
    }
    provokace(pole, n);
    if (strcmp(pole, "ahoj") == 0) {
      Serial.println("Co je moje jméno?");
      stavKomunikace = 1;
    }
    if (strcmp(pole, "zobraz") == 0) {
      nactiUdaje();
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
}
