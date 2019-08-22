/*
   ESP-01 pinout:

  GPIO 2 - DataIn
  GPIO 1 - LOAD/CS
  GPIO 0 - CLK

  ------------------------
  NodeMCU 1.0 pinout:

  D8 - DataIn
  D7 - LOAD/CS
  D6 - CLK
*/

#include "math.h"
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define NUM_MAX 4
#define ROTATE 90

// Si vous utilisez un module ESP-01, remplacez false par true
// for ESP-01 module
//  #define DIN_PIN 2 // D4
//  #define CS_PIN  3 // D9/RX
//  #define CLK_PIN 0 // D3

// for NodeMCU 1.0    
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 12  // D6


#include "max7219.h"
#include "fonts.h"



// DEBUT DE CONFIGURATION

bool enableFacebookCountFans = false;
bool enableYoutubeCountViews = false;
bool enableYoutubeCountVideos = false;
bool enableYoutubeCountSubscribers = false;

// =======================================================================
//              Votre config Wifi
// =======================================================================
const char* ssid     = "";             // SSID du réseau Wifi
const char* password = "";             // Mot de passe du réseau wifi

// =======================================================================
//            Votre config API youtube
// =======================================================================
String ytApiV3Key = "";            // YouTube Data API v3 généré ici: https://console.developers.google.com
String channelId = "";             // l'id de la chaine ciblée

// =======================================================================
//             Votre config Fan Facebook
// =======================================================================
// @see https://github.com/Cyrhades/Facebook_Fan_without_API
const char* fbFanHost = "";         // adresse de votre serveur (vous donnant l'information du nombre de fans sur votre page Facebbok)
const char* urlPageGetLikeFb = "";  // le nom du fichier (sur votre serveur vous donnant l'information du nombre de fans sur votre page Facebbok)

// =======================================================================
//             Config affichage du compteur
// =======================================================================
int loopRequest = 5; // interroger les serveurs (Youtube et Facebook) tout les n boucles 
int timeFixedInfo = 5; // en secondes nombre de temps ou les datas sont affichées de façon fixes

// FIN DE CONFIGURATION








// =======================================================================
//                    API youtube
// =======================================================================
const char* ytHost = "www.googleapis.com";
const int httpsPort = 443;
const int httpPort = 80;

// =======================================================================
//                  Démarrage du Module
// =======================================================================
void setup() 
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN,1);
  sendCmdAll(CMD_INTENSITY,8);
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift(" WiFi ...~",15,font,' ');
  
  // Si le wifi n'est pas encore connect�
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connecte: "); Serial.println(WiFi.localIP());
}

// =======================================================================
// Déclarations des variables globales
// =======================================================================
long viewCount = 0;         // Nombre de vues total sur la chaine Youtube
long subscriberCount = 0;   // Nombre d'abonnés sur la chaine Youtube
long videoCount = 0;        // Nombre de vidéos sur la chaine Youtube
long fanCount = 0;          // Nombre de fan sur la page Facebook

int cnt = 0;      // compteur pour boucle 
bool thingEnable = false;

// =======================================================================
// D�marrage du programme
// =======================================================================
void loop()
{
  // On interroge les serveurs qu'une fois tout les (loopRequest) 
  if(cnt<=0) {
    if (enableFacebookCountFans == true)
    {
      callJson("facebook", fbFanHost, urlPageGetLikeFb, true);
      thingEnable = true;
    }

    if (enableYoutubeCountSubscribers == true || enableYoutubeCountViews == true || enableYoutubeCountVideos == true)
    {
      callJson("youtube", ytHost, "/youtube/v3/channels?part=statistics&id=" + channelId + "&key=" + ytApiV3Key, true);
      thingEnable = true;
    }
  }
  cnt++;

  if (cnt >= 0 && thingEnable ==  true) {
    int del = timeFixedInfo*1000;
    int scrollDel = 30;
    char txt[10];

    // si affichage d'abonnés sur la chaine youtube
    if (enableYoutubeCountSubscribers == true) {
      printStringWithShift("  Abonnes: ", scrollDel, font, ' '); 
      printValueWithShift(subscriberCount,scrollDel,0);
      delay(del);
    }

    // si affichage de vues sur la chaine youtube
    if (enableYoutubeCountViews == true) {
      printStringWithShift("  Vues: ", scrollDel, font, ' ');
      printValueWithShift(viewCount,scrollDel,0);
      delay(del);
    }

    // si affichage du nombre de vidéos sur la chaine youtube
    if (enableYoutubeCountVideos == true) {
      printStringWithShift("  Videos: ", scrollDel, font, ' ');
      printValueWithShift(videoCount, scrollDel, 0);
      delay(del);
    }

    // si affichage du nombre de fans sur la page Facebook
    if (enableFacebookCountFans == true) {
      printStringWithShift("  Fans : ", scrollDel, font, ' ');
      printValueWithShift(fanCount, scrollDel, 0);
      delay(del);
    }

    // tout les (loopRequest) on remet le compteur à 0
    if (cnt > loopRequest) cnt = 0;
  }
  else {
     printStringWithShift("  Veuillez correctement configurer le code avant de le televerser, Erreur", 50, font, ' '); 
     delay(5000);
  }
}
// =======================================================================

// =======================================================================
//        PARTIE GESTION DE L'AFFICHAGE SUR LA MATRICE LED
// =======================================================================
int charWidth(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  return pgm_read_byte(data + 1 + ch * len);
}

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  scr[NUM_MAX*8] = 0;
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8+i+1] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  return w;
}

void printCharWithShift(unsigned char c, int shiftDelay, const uint8_t *data, int offs) 
{
  if(c < offs || c > MAX_CHAR) return;
  c -= offs;
  int w = showChar(c, data);
  for (int i=0; i<w+1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

void printStringWithShift(const char *s, int shiftDelay, const uint8_t *data, int offs)
{
  while(*s) printCharWithShift(*s++, shiftDelay, data, offs);
}

// =======================================================================
// printValueWithShift():
// converts int to string
// centers string on the display
// chooses proper font for string/number length
// can display sign - or +
void printValueWithShift(long val, int shiftDelay, int sign)
{
  const uint8_t *digits = digits5x7;       // good for max 5 digits
  if(val>1999999) digits = digits3x7;      // good for max 8 digits
  else if(val>99999) digits = digits4x7;   // good for max 6-7 digits
  String str = String(val);
  if(sign) {
    if(val<0) str=";"+str; else str="<"+str;
  }
  const char *s = str.c_str();
  int wd = 0;
  while(*s) wd += 1+charWidth(*s++ - '0', digits);
  wd--;
  int wdL = (NUM_MAX*8 - wd)/2;
  int wdR = NUM_MAX*8 - wdL - wd;
  //Serial.println(wd); Serial.println(wdL); Serial.println(wdR);
  s = str.c_str();
  while(wdL>0) { printCharWithShift(':', shiftDelay, digits, '0'); wdL--; }
  while(*s) printCharWithShift(*s++, shiftDelay, digits, '0');
  while(wdR>0) { printCharWithShift(':', shiftDelay, digits, '0'); wdR--; }
}

// =======================================================================


void callJson(String socialNetwork, String server, String url, bool secure)
{
  WiFiClientSecure client;
  // permettre le https
  if (secure) {
    client.setInsecure();
  }
  // Le serveur ne réponds pas
  if (client.connect(server, httpsPort)) {
  
    client.print(
        String("GET ") +
        url+ " HTTP/1.1\r\n" +
        "Host: " + server + "\r\n" +
        "User-Agent: ESP8266/1.1\r\n" +
        "Connection: close\r\n\r\n");
  
    int repeatCounter = 10;
    while (!client.available() && repeatCounter--)
    {
      Serial.println("y.");
      delay(500);
    }
    String line, buf = "";
    int startJson = 0;
    while (client.connected() && client.available())
    {
      line = client.readStringUntil('\n');
      if (line[0] == '{')
        startJson = 1;
      if (startJson)
      {
        for (int i = 0; i < line.length(); i++)
          if (line[i] == '[' || line[i] == ']')
            line[i] = ' ';
        buf += line + "\n";
      }
    }
    client.stop();
  
    DynamicJsonBuffer jsonBuf;
    JsonObject &root = jsonBuf.parseObject(buf);
     if (!root.success()) {
        Serial.println("parseObject() failed");
        printStringWithShift("json error!",30,font,' ');
        delay(10);
      } else {
        if(socialNetwork == "facebook") {
            fanCount       = root["count_fan"];
        }
        else if(socialNetwork == "youtube") {
            viewCount       = root["items"]["statistics"]["viewCount"];
            subscriberCount = root["items"]["statistics"]["subscriberCount"];
            videoCount      = root["items"]["statistics"]["videoCount"];
        }      
      }
   }
}
// =======================================================================
