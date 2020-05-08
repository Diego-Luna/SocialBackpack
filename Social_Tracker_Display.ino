
#include <Wire.h>                  // installed by default
#include <Adafruit_GFX.h>          // https://github.com/adafruit/Adafruit-GFX-Library
#include "Adafruit_LEDBackpack.h"  // https://github.com/adafruit/Adafruit_LED_Backpack
#include <ArduinoJson.h>           // https://github.com/bblanchon/ArduinoJson
#include "InstagramStats.h"       // https://github.com/witnessmenow/arduino-instagram-stats
#include "JsonStreamingParser.h"  // https://github.com/squix78/json-streaming-parser

// these libraries are included with ESP8266 support
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//////// facebook
#include <Ticker.h>
#define FACEBOOK_HOST "graph.facebook.com"
#define FACEBOOK_PORT 443
#define PAGE_ID "ElIDdelapaguinaDeFacebook"
#define ACCESS_TOKEN "Token"

const char* facebookGraphFingerPrint = "93 6F 91 2B AF AD 21 6F A5 15 25 6E 57 2C DC 35 A1 45 1A A5";


Ticker tickerRequest;
bool requestNewState = true;
int pageFansCount = 0;

#define REQUEST_INTERVAL 500*1*1 //1000*60*1


////////Youtube
#include <YoutubeApi.h>            // https://github.com/witnessmenow/arduino-youtube-api

// google API key
// create yours: https://support.google.com/cloud/answer/6158862?hl=en
#define API_KEY "ApiDeGoogle"

// youtube channel ID
// find yours: https://support.google.com/youtube/answer/3250431?hl=en
#define CHANNEL_ID "ElIDdeCanal de Youtube"

int subscriberCount; // create a variable to store the subscriber count


//------- Replace the following! ------
char ssid[] = "Red";       // your network SSID (name)
char password[] = "contraseña";  // your network key

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);
InstagramStats instaStats(client);

WiFiClientSecure tlsClient;  //facebook


unsigned long api_delay = 1 * 0; //time between api requests (1mins)
unsigned long api_due_time;

long subs = 0;

//Inputs

String userName = "NombreDeUsario";    // from their instagram url https://www.instagram.com/userName/

bool haveBearerToken = false;

// label the displays with their i2c addresses
struct {
  uint8_t           addr;         // I2C address
  Adafruit_7segment seg7;         // 7segment object
} disp[] = {
  { 0x71, Adafruit_7segment() },  // High digits Youtube
  { 0x70, Adafruit_7segment() },  // Low digits Youtube
  { 0x74, Adafruit_7segment() },  // High digits Instagram
  { 0x73, Adafruit_7segment() },  // Low digits Instagram
  { 0x75, Adafruit_7segment() },  // High digits Facebook
  { 0x72, Adafruit_7segment() }   // Low digits Facebook
};

////////////// facebook
void request(){
  requestNewState = true;
}
String makeRequestGraph(){
  if (!tlsClient.connect(FACEBOOK_HOST, FACEBOOK_PORT)) {
    Serial.println("Host connection failed");
    return "";    
  }
  
  String params = "?pretty=0&fields=fan_count&access_token="+String(ACCESS_TOKEN);
  String path = "/v7.0/" + String(PAGE_ID);
  String url = path + params;
  Serial.print("requesting URL: ");
  Serial.println(url);

  String request = "GET " + url + " HTTP/1.1\r\n" +
    "Host: " + String(FACEBOOK_HOST) + "\r\n\r\n";
  
  tlsClient.print(request);

  String response = "";
  String chunk = "";  
  
  do {
    if (tlsClient.connected()) {
      delay(5);
      chunk = tlsClient.readStringUntil('\n');
      if(chunk.startsWith("{")){
        response += chunk;
      }
    }
  } while (chunk.length() > 0);
    
  Serial.print(" Message ");
  Serial.println(response);  

  return response;
}

int getPageFansCount(){
  String response = makeRequestGraph();  
  
  const size_t bufferSize = JSON_OBJECT_SIZE(2) + 40;
  DynamicJsonBuffer jsonBuffer(bufferSize);  
  
  JsonObject& root = jsonBuffer.parseObject(response);
  
  int fanCount = root["fan_count"];  
  return fanCount;
}
////////////



void setup() {

  Serial.begin(115200);
  
  for(uint8_t i=0; i<6; i++) {       // Initialize displays
    disp[i].seg7.begin(disp[i].addr);
    disp[i].seg7.clear();
    disp[i].seg7.writeDisplay();
  }

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  haveBearerToken = true;
  //getTwitterStats(screenName);

  tickerRequest.attach_ms(REQUEST_INTERVAL, request);
  //showNumber(pageFansCount);
  
}

void loop() {
 if (millis() > api_due_time)  {
    YoutubeSub();
    getInstagramStatsForUser();
    Facebook();
    api_due_time = millis() + api_delay;
  }
  
}

void getInstagramStatsForUser() {
  Serial.println(" ");
  Serial.println("-----------Instagram-----------");

  Serial.println("Getting instagram user stats for " + userName );
  InstagramUserStats response = instaStats.getUserStats(userName);
  Serial.print("Number of followers: ");
  Serial.println(response.followedByCount);
  Serial.println("-----------Instagram-----------");

  uint16_t hi = response.followedByCount / 10000, // Value on left (high digits) display
           lo = response.followedByCount % 10000; // Value on right (low digits) display
      disp[2].seg7.print(hi, DEC);   // Write values to each display...
      disp[3].seg7.print(lo, DEC);

      // print() does not zero-pad the displays; this may produce a gap
      // between the high and low sections. Here we add zeros where needed...
      if(hi) {
        if(lo < 1000) {
          disp[3].seg7.writeDigitNum(0, 0);
          if(lo < 100) {
            disp[3].seg7.writeDigitNum(1, 0);
            if(lo < 10) {
              disp[3].seg7.writeDigitNum(3, 0);
            }
          }
        }
       } else {
         disp[2].seg7.clear(); // Clear 'hi' display
        }
       disp[2].seg7.writeDisplay(); // Push data to displays
       disp[3].seg7.writeDisplay();
}

void YoutubeSub(){
  if(api.getChannelStatistics(CHANNEL_ID))
    {
      Serial.println(" ");
      Serial.println("-----------Youtube-----------");
      Serial.println("---------Stats---------");
      Serial.print("Subscriber Count: ");
      Serial.println(api.channelStats.subscriberCount);
      // Probably not needed :)
      //Serial.print("hiddenSubscriberCount: ");
      //Serial.println(api.channelStats.hiddenSubscriberCount);
      Serial.println("------------------------");
      Serial.println("-----------Youtube-----------");

      
      subscriberCount = api.channelStats.subscriberCount;
      
      uint16_t hi = subscriberCount / 10000, // Value on left (high digits) display
               lo = subscriberCount % 10000; // Value on right (low digits) display
      disp[0].seg7.print(hi, DEC);   // Write values to each display...
      disp[1].seg7.print(lo, DEC);

      // print() does not zero-pad the displays; this may produce a gap
      // between the high and low sections. Here we add zeros where needed...
      if(hi) {
        if(lo < 1000) {
          disp[1].seg7.writeDigitNum(0, 0);
          if(lo < 100) {
            disp[1].seg7.writeDigitNum(1, 0);
            if(lo < 10) {
              disp[1].seg7.writeDigitNum(3, 0);
            }
          }
        }
       } else {
         disp[0].seg7.clear(); // Clear 'hi' display
        }
       disp[0].seg7.writeDisplay(); // Push data to displays
       disp[1].seg7.writeDisplay();
      }
  }

void Facebook(){
  if(requestNewState){
    Serial.println(" ");
    Serial.println("-----------Facebook-----------");
    Serial.println("Request new State");               
       
    int pageFansCountTemp = getPageFansCount();
    
    Serial.print("Page fans count: ");
    Serial.println(pageFansCountTemp);

    Serial.println("-----------Facebook-----------");
    
    if(pageFansCountTemp <= 0){
      
      Serial.println("Error requesting data");    
      
    }else{
      pageFansCount = pageFansCountTemp;
      requestNewState = false; 

      //showNumber(pageFansCount);
    }

  uint16_t hi = pageFansCountTemp / 10000, // Value on left (high digits) display
           lo = pageFansCountTemp % 10000; // Value on right (low digits) display
      disp[4].seg7.print(hi, DEC);   // Write values to each display...
      disp[5].seg7.print(lo, DEC);

      // print() does not zero-pad the displays; this may produce a gap
      // between the high and low sections. Here we add zeros where needed...
      if(hi) {
        if(lo < 1000) {
          disp[5].seg7.writeDigitNum(0, 0);
          if(lo < 100) {
            disp[5].seg7.writeDigitNum(1, 0);
            if(lo < 10) {
              disp[5].seg7.writeDigitNum(3, 0);
            }
          }
        }
       } else {
         disp[4].seg7.clear(); // Clear 'hi' display
        }
       disp[4].seg7.writeDisplay(); // Push data to displays
       disp[5].seg7.writeDisplay();
    
  }  
}
