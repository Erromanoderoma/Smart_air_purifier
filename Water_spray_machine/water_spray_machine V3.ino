#include <ESP8266WiFi.h>
#include <espnow.h>

const int RELAY_LED = 14;
const int RELAY_PUMP = 12;
const int BUZZER = 5;
const int Liquid_Level_Sensor = 0;
bool TRIGGER;
int Water_detected;
bool water_trigger;

unsigned long lastReceivedTime = 0;
const unsigned long timeout = 5000;  // 5 วินาที

typedef struct struct_message {
  int pm;
  int gas;
  bool trigger;
} struct_message;

struct_message myData;

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.println("Data received");
  lastReceivedTime = millis();
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_LED, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(Liquid_Level_Sensor, INPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(RELAY_LED, 1);
  digitalWrite(RELAY_PUMP, 1);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  WATER();

  unsigned long currentTime = millis();
  if (currentTime - lastReceivedTime > timeout) {
    TRIGGER = false; //หากไม่มีข้อมูลภายในเวลาที่กำหนด
  } else {
    TRIGGER = myData.trigger; // ควบคุมด้วยข้อมูลที่ได้รับ
  }

  if (water_trigger) {
    if (TRIGGER) {
      digitalWrite(RELAY_PUMP, 1);
      digitalWrite(RELAY_LED, 1);
    } else {
      digitalWrite(RELAY_PUMP, 0);
      digitalWrite(RELAY_LED, 0);
    }
  } else {
    digitalWrite(RELAY_PUMP, 0);
    digitalWrite(RELAY_LED, 0);
  }

  delay(1000);
}

void WATER() {
  Water_detected = analogRead(Liquid_Level_Sensor);
  if (Water_detected > 1000) {
    water_trigger = true;
    Serial.println("Water detected");
  } else {
    water_trigger = false;
    Serial.println("No Water");
    tone(BUZZER, 1500);
    delay(250);
    noTone(BUZZER);
  }
  Serial.print("Water_detected: ");
  Serial.println(Water_detected);
}
