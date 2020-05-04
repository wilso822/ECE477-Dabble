/**
   BasicHTTPSClient.ino

    Created on: 20.08.2018

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>

// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const uint8_t fingerprint[20] = {0x66, 0x7A, 0x9F, 0x2B, 0x4D, 0x74, 0xDE, 0x41, 0xD2, 0x59, 0x14, 0xE2, 0x94, 0xFE, 0xF4, 0x82, 0x56, 0x02, 0x18, 0x55};
//const char board[81] = "attackers00r0a000000a0r000t0ism0robe00human0l000s0k00l00never0e0rot0000r000000000";
char receivedChars[100];//serial buffer for request url
boolean newData = false;
char buf[100];
char html[1000];
int stringlen;
String url = "https://engineering.purdue.edu/477grp15/var/www/cgi-bin/boardCheck.py?board=";

ESP8266WiFiMulti WiFiMulti;

void setup() {

  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("NETGEAR570", "593FD8D8D9FE533F");
}

void loop() {
  if(!newData){
    recvWithEndMarker();
  }
  else{
    showNewData();
    receivedChars[stringlen-1] = '\0';
    if ((WiFiMulti.run() == WL_CONNECTED)) {
  
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  
      client->setFingerprint(fingerprint);
  
      HTTPClient https;
  
      if (https.begin(*client, url + receivedChars)) {  // HTTPS
       // start connection and send HTTP header
        int httpCode = https.GET();
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
            
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            if(payload[payload.lastIndexOf('!') - 10] == 'd'){
              Serial.println("1");
            }
            else{
              Serial.println("0");
            }
          }
        } else {
          Serial.printf("GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        Serial.printf("Unable to connect\n");
      }
    }
  }
  
}

void recvWithEndMarker() {
 static byte ndx = 0;
 char endMarker = '\n';
 char rc;
 
 while (Serial.available() > 0 && newData == false) {
  rc = Serial.read();

 if (rc != endMarker) {
  receivedChars[ndx] = rc;
  ndx++;
  if (ndx >= 100) {
    ndx = 100 - 1;
  }
 }
 else {
  receivedChars[ndx] = '\0'; // terminate the string
  stringlen = ndx;
  ndx = 0;
  newData = true;
 }
 }
}

void showNewData() {
 if (newData == true) {
 newData = false;
 }
}
