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
volatile bool relatka[8]={0,0,0,0,0,0,0,0};
volatile bool uprava=false;
unsigned long cas;
void nastavRka(){
    for(int i=4;i<=11;i++){
  digitalWrite(i,!relatka[i-4]);
  }
}

SoftwareSerial mySerial(SRX, STX); // RX, TX>

// The setup() function runs once each time the micro-controller starts
void setup()
{
	for(int i=4;i<=11;i++){
  pinMode(i,OUTPUT);
  digitalWrite(i,HIGH);
	}
  
	Serial.begin(115200);
  mySerial.begin(9600);
  cas=millis();
  mySerial.println("RX:?");

}

// Add the main program code into the continuous loop() function
void loop()
{	
  if(millis()-cas>=100){
    cas=millis();
    if (Serial.available()){
    byte n = Serial.available();
    char pole[n];
    for (byte i = 0; i < n; i++) {
      pole[i] = Serial.read();
      mySerial.write(pole[i]);
    }
    pole[n - 1] = '\0';
    
    provokace(pole,n);
  }
  if (mySerial.available() > 0) //Checks is there any data in buffer
  {
    byte n = mySerial.available();
    char pole[n];
    Serial.print("#");
    for (byte i = 0; i < n; i++) {
      pole[i] = mySerial.read();
      Serial.write(pole[i]);
    }
    pole[n - 1] = '\0';
    //Serial.println(pole);
    
    provokace(pole,n);

  }
  
  }
  if(uprava){
  Serial.println("nastavuji");
    nastavRka();
    uprava=false;
  }
	


}
void prevedNaPiny(byte cislo){
	for(byte i=0;i<8;i++){
		relatka[i]=0b00000001&cislo;
		cislo=cislo>>1;
	}
	nastavRka();
	
}
void provokace(char pole[], byte n){
	if(pole[0]=='P'){
  if(n>=3)prevedNaPiny(pole[1]);
  else prevedNaPiny(0);
  mySerial.println("y");
	}
	//Serial.println(pole);
	if(pole[0]=='R'){
		switch(pole[1]-48){
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:{
				char blbyJmeno[10];
				bool akce=false;
				sprintf(blbyJmeno,"R%c:ON",pole[1]);
				if(strstr(pole,blbyJmeno)){
					akce=true;
					//Serial.printf("ano zapnu ti ho %s",pole);
				}
				if(akce!=relatka[pole[1]-48-1]){
					relatka[pole[1]-48-1]=akce;
					uprava=true;
				}
				Serial.println("yes");
        mySerial.println("yes");
				
				//Serial.println(pole);
				
				break;
			}
			case 40:{
				char blbyJmeno[10];
				sprintf(blbyJmeno,"R%c:ON",pole[1]);
				if(strstr(pole,blbyJmeno)){
					for(byte pohoda=0;pohoda<8;pohoda++){
						relatka[pohoda]=1;
					}
					
					
				}
				else{
					for(byte pohoda=0;pohoda<8;pohoda++){
						relatka[pohoda]=0;
					}
				}
				uprava=1;
				Serial.println(pole);
       mySerial.println("yes");
				
				break;
			}
			default:{
				Serial.println("#to je divny!");
				break;
			}
		}
	}
}
