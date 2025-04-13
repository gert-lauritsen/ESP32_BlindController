// ESP32 Blind Controller with DRV8825 and MQTT Auto-Discovery for Home Assistant
//remeber to change #define MQTT_MAX_PACKET_SIZE 512 in PubSubClient.h 
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define STEP_PIN 18
#define DIR_PIN 19
#define ENABLE_PIN 21
#define LIMIT_TOP_PIN 34
#define LIMIT_BOTTOM_PIN 35

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;


const char* room = "living_room"; // <--- Change this to the desired room name

WiFiClient espClient;
PubSubClient client(espClient);

long topPosition = 0;
long bottomPosition = 0;
long currentPosition = 0;

bool calibrated = false;

#define EEPROM_SIZE 64

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void publishDiscoveryConfig() {
  String topic = "homeassistant/cover/" + String(room) + "_blind/config";

  StaticJsonDocument<512> doc;
  String displayName = String(room);
  displayName.replace("_", " ");

  doc["name"] = displayName + " Blind";
  doc["command_topic"] = "home/blind/" + String(room) + "/set";
  doc["position_topic"] = "home/blind/" + String(room) + "/state";
  doc["set_position_topic"] = "home/blind/" + String(room) + "/set";
  doc["unique_id"] = "esp32_blind_" + String(room);
  doc["device_class"] = "blind";

  char buffer[512];
  size_t len = serializeJson(doc, buffer);

  bool success = client.publish(topic.c_str(), buffer, true);
  Serial.print("Discovery publish result: ");
  Serial.println(success ? "SUCCESS" : "FAILED");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  String setTopic = String("home/blind/") + room + "/set";
  String calibrateTopic = String("home/blind/") + room + "/calibrate";

  if (String(topic) == setTopic) {
    if (msg == "open") moveTo(topPosition);
    else if (msg == "close") moveTo(bottomPosition);
    else {
      int percent = msg.toInt();
      long range = topPosition - bottomPosition;
      if (range == 0) return;
      long target = bottomPosition + ((range * percent) / 100);
      moveTo(target);
    }
  }  
  if (String(topic) == calibrateTopic) {
    calibrate();
  }
  if (String(topic) == "home/blind/" + String(room) + "/save_bottom") {
    bottomPosition = currentPosition;
    saveLimits();
    Serial.println("Bottom position saved.");
  }
  if (String(topic) == "home/blind/" + String(room) + "/save_top") {
    topPosition = currentPosition;
    saveLimits();
    Serial.println("Top position saved.");
  }
  if (String(topic) == "home/blind/" + String(room) + "/set_position") {
    Serial.println("set_position");
    int target = String((char*)payload).toInt();
    moveTo(target);
  } 
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Blind", mqtt_user, mqtt_pass)) {
      String setStatus = String("home/blind/") + room + "/status";
      bool success = client.publish(setStatus.c_str(), "online",true);
      Serial.print("Mqtt Online result: ");      
      String setTopic = String("home/blind/") + room + "/set";
      String calibrateTopic = String("home/blind/") + room + "/calibrate";

      client.subscribe(setTopic.c_str());
      client.subscribe(calibrateTopic.c_str());
      client.publish("home/blind/status", "online", true);
      publishDiscoveryConfig();
    } else {
      delay(5000);
    }
  }
}

void moveTo(long target) {
  digitalWrite(ENABLE_PIN, LOW);
  digitalWrite(DIR_PIN, target > currentPosition ? HIGH : LOW);

  long steps = abs(target - currentPosition);
  for (long i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(800);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(800);
    currentPosition += (target > currentPosition) ? 1 : -1;
  }

  digitalWrite(ENABLE_PIN, HIGH);
  publishState();
}

void calibrate() {
  digitalWrite(ENABLE_PIN, LOW);

  digitalWrite(DIR_PIN, LOW);
  while (digitalRead(LIMIT_BOTTOM_PIN) == HIGH) {
    stepOnce();
    currentPosition--;
  }
  bottomPosition = currentPosition;

  delay(500);

  digitalWrite(DIR_PIN, HIGH);
  while (digitalRead(LIMIT_TOP_PIN) == HIGH) {
    stepOnce();
    currentPosition++;
  }
  topPosition = currentPosition;

  digitalWrite(ENABLE_PIN, HIGH);
  saveLimits();
  calibrated = true;
  publishState();
}

void stepOnce() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(800);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(800);
}

void saveLimits() {
  EEPROM.writeLong(0, bottomPosition);
  EEPROM.writeLong(4, topPosition);
  EEPROM.commit();
}

void loadLimits() {
  bottomPosition = EEPROM.readLong(0);
  topPosition = EEPROM.readLong(4);
  calibrated = true;
}

void publishState() {
  String stateTopic = String("home/blind/") + room + "/state";

  long range = topPosition - bottomPosition;
  if (range == 0) {
    client.publish(stateTopic.c_str(), "0", true);
    return;
  }

  int percent = (currentPosition - bottomPosition) * 100 / range;
  percent = constrain(percent, 0, 100);  // just in case
  client.publish(stateTopic.c_str(), String(percent).c_str(), true);
}


void setup() {
  Serial.begin(115200);
  Serial.println("-------------------------------------------------------------------");  
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(LIMIT_TOP_PIN, INPUT_PULLUP);
  pinMode(LIMIT_BOTTOM_PIN, INPUT_PULLUP);

  digitalWrite(ENABLE_PIN, HIGH);

  EEPROM.begin(EEPROM_SIZE);
  loadLimits();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Setup Completed");
  Serial.println("-------------------------------------------------------------------");  
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
}
