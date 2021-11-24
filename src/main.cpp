#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

#define             EEPROM_LENGTH 1024
#define             RESET_PIN 0
ESP8266WebServer    webServer(80);
char                eRead[30];
char                ssid[30];
char                password[30];
char                mqttServer[30];
const int           mqttPort = 1883;
int relay = 15;

String responseHTML = ""
    "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body><center>"
    "<p>Captive Sample Server App</p>"
    "<form action='/save'>"
    "<p><input type='text' name='ssid' placeholder='SSID' onblur='this.value=removeSpaces(this.value);'></p>"
    "<p><input type='text' name='password' placeholder='WLAN Password'></p>"
    "<p><input type='text' name='mqttServer' placeholder='mqttServer address'></p>"
    "<p><input type='submit' value='Submit'></p></form>"
    "<script>function removeSpaces(string) {"
    "   return string.split(' ').join('');"
    "}</script></html>";

WiFiClient espClient;
PubSubClient client(espClient);

char topic[30] = "deviceid/Yuning/cmd/lamp";

// Saves string to EEPROM
void SaveString(int startAt, const char* id) { 
    for (byte i = 0; i <= strlen(id); i++) {
        EEPROM.write(i + startAt, (uint8_t) id[i]);
    }
    EEPROM.commit();
}

// Reads string from EEPROM
void ReadString(byte startAt, byte bufor) {
    for (byte i = 0; i <= bufor; i++) {
        eRead[i] = (char)EEPROM.read(i + startAt);
    }
}

void save(){
    Serial.println("button pressed");
    Serial.println(webServer.arg("ssid"));
    SaveString( 0, (webServer.arg("ssid")).c_str());
    SaveString(30, (webServer.arg("password")).c_str());
    SaveString(60, (webServer.arg("mqttServer")).c_str());
    webServer.send(200, "text/plain", "OK");
    ESP.restart();
}

void configWiFi() {
    const byte DNS_PORT = 53;
    IPAddress apIP(192, 168, 1, 1);
    DNSServer dnsServer;
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Yun");     // change this to your portal SSID
    
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/save", save);

    webServer.onNotFound([]() {
        webServer.send(200, "text/html", responseHTML);
    });
    webServer.begin();
    while(true) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        yield();
    }
}

void load_config_wifi() {
    ReadString(0, 30);
    if (!strcmp(eRead, "")) {
        Serial.println("Config Captive Portal started");
        configWiFi();
    } else {
        Serial.println("IOT Device started");
        strcpy(ssid, eRead);
        ReadString(30, 30);
        strcpy(password, eRead);
        ReadString(60,30);
        strcpy(mqttServer, eRead);
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived : ");
  Serial.print(topic);
  Serial.print("\t");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
 
  if ((char)payload[1] == 'n') {
    digitalWrite(relay, HIGH);
  } else if ((char)payload[1] == 'f'){
    digitalWrite(relay, LOW);  
  }
  // on off를 통째로 받아서 사용하고 싶어 buf를 사용해보고, String을 사용해 봤었습니다.
  // 하지만 방법 잘 찾지 못하여 on off에서 동시에 존재하는 o가 아닌 n과 f를 사용해 on off 시키도록 했습니다.
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(EEPROM_LENGTH);
  pinMode(relay, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  load_config_wifi();

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if(i++ > 15) {
          configWiFi();
      }
  }
  Serial.print("Connected to "); Serial.println(ssid);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("Yuning")) {
      Serial.println("connected");  
    } else {
      Serial.print("failed with state "); Serial.println(client.state());
      delay(2000);
    }
  }
  
  client.subscribe(topic);
}

void loop() {
  // put your main code here, to run repeatedly:
  client.loop();
}