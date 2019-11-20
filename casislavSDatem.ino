#include "ESP8266WiFi.h"
#include "NTPClient.h"
#include <WiFiUdp.h>

class cas {
  public:
  int8_t sekundy, minuty, hodiny, dny, mesice;
  int16_t roky;
  int8_t * polak[5]{ &sekundy, &minuty, &hodiny, &dny, &mesice };
  cas() {
    sekundy = 0;
    minuty = 0;
    hodiny = 0;
    dny = 0;
    mesice = 0;
    roky = 0;
  }
  cas(uint8_t sekundi, uint8_t minuti, uint8_t hodnini) {
    sekundy = sekundi;
    minuty = minuti;
    hodiny = hodnini;
    dny = 0;
    mesice = 0;
    roky = 0;
  }
  cas(uint8_t sekundi, uint8_t minuti, uint8_t hodnini, uint8_t dni, uint8_t mesici, uint16_t roki) {
    sekundy = sekundi;
    minuty = minuti;
    hodiny = hodnini;
    dny = dni;
    mesice = mesici;
    roky = roki;
  }
  cas operator - (cas dalsi) {
    cas vysledny;
    if (sekundy - dalsi.sekundy < 0) {
      if (minuty - 1 < 0) {
        if (hodiny - 1 >= 0) {
          hodiny--;
          minuty += 59;
          sekundy += 60;
        }
      }
      else
      {
        minuty--;
        sekundy += 60;
      }
    }
    vysledny.sekundy = sekundy - dalsi.sekundy;
    if (minuty - 1 < 0) {
      if (hodiny - 1 >= 0) {
        hodiny--;
        minuty += 60;
      }
    }
    vysledny.minuty = minuty - dalsi.minuty;
    vysledny.hodiny = hodiny - dalsi.hodiny;
    return vysledny;
  }
  bool operator == (cas dalsi) {
    if (dny != 0 && dalsi.dny != 0) {
      if (sekundy == dalsi.sekundy&&minuty == dalsi.minuty&&hodiny == dalsi.hodiny&&dny == dalsi.dny&&mesice == dalsi.mesice&&roky == dalsi.roky) {
        return true;
      }
      return false;
    }
    else
    {
      if (sekundy == dalsi.sekundy&&minuty == dalsi.minuty&&hodiny == dalsi.hodiny) {
        return true;
      }
      return false;
    }
  }
  bool operator < (cas dalsi) {
    if (dny != 0 && dalsi.dny != 0) {
      if (roky < dalsi.roky) {
        return true;
      }
      else {
        if (roky == dalsi.roky) {
          for (int8_t i = 4; i >= 0; i--) {
            if (polak[i] < dalsi.polak[i]) {
              return true;
            }
            else {
              if (polak[i] == dalsi.polak[i]) {
                continue;
              }
              return false;
            }
          }
        }
        return false;
      }
    }
    else
    {
      for (int8_t i = 2; i >= 0; i--) {
        if (polak[i] < dalsi.polak[i]) {
          return true;
        }
        else {
          if (polak[i] == dalsi.polak[i]) {
            continue;
          }
          return false;
        }
      }
      return false;
    }
  }
  bool operator <= (cas dalsi) {
    if (dny != 0 && dalsi.dny != 0) {
      if (roky < dalsi.roky) {
        return true;
      }
      else {
        if (roky == dalsi.roky) {
          for (int8_t i = 4; i >= 0; i--) {
            if (polak[i] <= dalsi.polak[i]) {
              return true;
            }
            else {
              if (polak[i] == dalsi.polak[i]) {
                continue;
              }
              return false;
            }
          }
        }
        return true;
      }
    }
    else
    {
      for (int8_t i = 2; i >= 0; i--) {
        if (polak[i] <= dalsi.polak[i]) {
          return true;
        }
        else {
          if (polak[i] == dalsi.polak[i]) {
            continue;
          }
          return false;
        }
      }
      return true;
    }
  }
  bool operator > (cas dalsi) {
    if (dny != 0 && dalsi.dny != 0) {
      if (roky > dalsi.roky) {
        return true;
      }
      else {
        if (roky == dalsi.roky) {
          for (int8_t i = 4; i >= 0; i--) {
            if (polak[i] > dalsi.polak[i]) {
              return true;
            }
            else {
              if (polak[i] == dalsi.polak[i]) {
                continue;
              }
              return false;
            }
          }
        }
        return false;
      }
    }
    else
    {
      for (int8_t i = 2; i >= 0; i--) {
        if (polak[i] > dalsi.polak[i]) {
          return true;
        }
        else {
          if (polak[i] == dalsi.polak[i]) {
            continue;
          }
          return false;
        }
      }
      return false;
    }
  }
  bool operator >= (cas dalsi) {
    if (dny != 0 && dalsi.dny != 0) {
      if (roky >= dalsi.roky) {
        return true;
      }
      else {
        if (roky == dalsi.roky) {
          for (int8_t i = 4; i >= 0; i--) {
            if (polak[i] >= dalsi.polak[i]) {
              return true;
            }
            else {
              if (polak[i] == dalsi.polak[i]) {
                continue;
              }
              return false;
            }
          }
        }
        return true;
      }
    }
    else
    {
      for (int8_t i = 2; i >= 0; i--) {
        if (polak[i] >= dalsi.polak[i]) {
          return true;
        }
        else {
          if (polak[i] == dalsi.polak[i]) {
            continue;
          }
          return false;
        }
      }
      return true;
    }
  }
  bool operator != (cas dalsi) {
    if (dny != 0 && dalsi.dny != 0) {
      if (sekundy == dalsi.sekundy&&minuty == dalsi.minuty&&hodiny == dalsi.hodiny&&dny == dalsi.dny&&mesice == dalsi.mesice&&roky == dalsi.roky) {
        return false;
      }
      return true;
    }
    else
    {
      if (sekundy == dalsi.sekundy&&minuty == dalsi.minuty&&hodiny == dalsi.hodiny) {
        return false;
      }
      return true;
    }
  }
};
//const char* neco = "2.4GHz_nech_me_byt";
//const char* peco = "01189998819991197253";
// Replace with your network credentials
const char* ssid     = "2.4GHz_nech_me_byt";
const char* password = "01189998819991197253";
cas systemak;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
//timeClient.setTimeOffset(3600);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
}
void loop() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  systemak.hodiny=timeClient.getHours();
  systemak.minuty=timeClient.getMinutes();
  systemak.sekundy=timeClient.getSeconds();
  delay(1000);
}
