#include <Arduino.h> 
#include "WiFiManager.h"
#include <WebSocketsClient.h>
#include <Hash.h>
#include <EEPROM.h>

#define BLINK_COUNT 5
#define Reset D2
#define TARGET_PIN D4

char host[] = "ininfluent.herokuapp.com";
int port = 80;
int brightness = 255;     // how bright the LED is
int fadeAmount = 35;      // how many points to fade the LED by
int counter = 0;
float pulse = 0.7;
char wall[32];

WebSocketsClient webSocket;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}


void turnOffWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
}

void blink() {
  while (true) {
    analogWrite(TARGET_PIN, brightness);
    brightness = brightness - (fadeAmount * pulse);

    if (brightness <= 10 && counter == 0) {
      brightness = 255 ;

      counter = 1;
      fadeAmount = 5;
    }
    else if (brightness <= 10 && counter == 1) {
      delay(32 * 10);
      brightness = 255;
      counter = 0;
      fadeAmount = 35;
    }
    delay(2 * 10);
  }
}


void onRequest() {
  turnOffWiFi();
  Serial.println("Yes :) Lets blynk and turn off WiFi!");
  blink();
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      break;
    case WStype_TEXT:
      String message = (char*)payload;
      if (message == "request") {
        onRequest();
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  attachInterrupt(digitalPinToInterrupt(Reset), Clean, HIGH);
  Serial.setDebugOutput(false);
  pinMode(TARGET_PIN, OUTPUT);
  pinMode(Reset, INPUT);
  digitalWrite(TARGET_PIN, false);
  // event handler
  webSocket.onEvent(webSocketEvent);
  //  webSocket.begin(host, port, "/ws");
  webSocket.begin(host, port);
  webSocket.setReconnectInterval(500);
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  if ( esid != wall ) {
    Serial.println("Connessione in corso");
    WiFi.begin(esid.c_str(), epass.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    WiFiManager wifiManager;
    wifiManager.autoConnect("cUSBo");
    wifiManager.setAPCallback(configModeCallback);
    if (!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(1000);
    }
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println("provo a connettermi al webserver....");
    String qsid = WiFi.SSID() ;
    String qpass = WiFi.psk();
    Serial.println("writing eeprom ssid:");
    for (int i = 0; i < qsid.length(); ++i)
    {
      EEPROM.write(i, qsid[i]);
      Serial.print("Wrote: ");
      Serial.println(qsid[i]);
    }
    Serial.println("writing eeprom pass:");
    for (int i = 0; i < qpass.length(); ++i)
    {
      EEPROM.write(32 + i, qpass[i]);
      Serial.print("Wrote: ");
      Serial.println(qpass[i]);
    }
    EEPROM.commit();
  }
}

void loop() {
  webSocket.loop();
}

void Clean() {
  Serial.println("Cleaning WiFi");
  for (int i = 0; i <= 512; i++) {
    Serial.println(i);
    EEPROM.write(i, 0);
    EEPROM.commit();
    if (i == 512) {
      Serial.println("WiFi Cleaned");
      ESP.reset();
    }
  }
}
