#include <esp_now.h>
#include <WiFi.h>

#define MQ_9 36
#define BUZZER 15
// กำหนดขาสำหรับ Serial1
#define RXD1 16  // RX Pin สำหรับ Serial1
#define TXD1 17  // TX Pin สำหรับ Serial1


TaskHandle_t Task1;
TaskHandle_t Task2;


float sensorVoltage;
float sensorValue;
float GAS__READ_VALVE2 = 0;  /// cherry
int countTrigger2 = 0;       // ตัวแปรสำหรับนับจำนวนครั้ง

unsigned long lastReceivedTime = 0;    // เวลาที่ได้รับข้อความล่าสุด
unsigned long timeoutDuration = 5000;  // ระยะเวลารอ (5,000 ms = 5 วินาที)
int pmValue = 0;                       // ตัวแปรเก็บค่า PM ที่รับมา


unsigned long lastGasResetTime2 = 0;  // เก็บเวลาครั้งสุดท้ายที่เริ่มหน่วงเวลา
bool isGasResetDelayActive = false;   // สถานะว่ากำลังหน่วงเวลาอยู่หรือไม่

int pm2_5_value = 0;  // ตัวแปรสำหรับเก็บค่า pm2_5
String incomingData;
int GAS__READ_VALVE = 0;
boolean GAS_TRIGGER2 = false;

int PM_SENSOR;
int PM_VALVE = 1;  //ค่ากำหนด PM
int forpm_trigger = 0;

// REPLACE WITH YOUR ESP RECEIVER'S MAC ADDRESS
uint8_t broadcastAddress1[] = { 0x08, 0xA6, 0xF7, 0xB0, 0xA9, 0x34 };  //cofee
uint8_t broadcastAddress2[] = { 0xC4, 0xD8, 0xD5, 0x2D, 0x0A, 0xF0 };  //cola
//uint8_t broadcastAddress2[] = { 0xCC, 0x7B, 0x5C, 0xF0, 0x98, 0x18 };  //cofee NEW



typedef struct struct_message {
  int pm;        ///S
  int gas;       ///S
  bool trigger;  ///S
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  lastReceivedTime = millis();
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  // Serial.print("Char: ");
  // Serial.println(myData.a);
  // Serial.print("Recv_GAS: ");
  // Serial.println(myData.gas);
  Serial.println();
}



void setup() {
  // เริ่มต้น Serial สำหรับการดีบักที่ 115200
  Serial.begin(115200);
  Serial1.println("ESP32 Receiver (UART1) started.");

  // เริ่มต้น Serial1 ที่ขา RXD1 และ TXD1 โดยกำหนด Baud rate ที่ 115200
  Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);


  pinMode(BUZZER, OUTPUT);
  pinMode(MQ_9, INPUT);


  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  esp_now_register_send_cb(OnDataSent);

  // register peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // register first peer

  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // register second peer
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  /// register third peer
  /* memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }*/

  ///// ____ CORD ____  /////
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
    Task1code, /* Task function. */
    "Task1",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
    Task2code, /* Task function. */
    "Task2",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task2,    /* Task handle to keep track of created task */
    1);        /* pin task to core 1 */
}




void loop() {
  delay(1);
}

//____________________________C O D E 1___________________________________

void Task1code(void* pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    // ตรวจสอบว่ามีข้อมูลเข้ามาที่ Serial1 หรือไม่
    /*if (Serial1.available()) {
      // อ่านข้อมูลจาก Serial1 และแสดงบน Serial Monitor
      String incomingData = Serial1.readString();
      Serial.print("Received on UART1: ");
      Serial.println(incomingData);
    }*/
    /// debug จำลอง PM


    serial_ESP32_Sent();
    SENDER();
    sensor_MQ_9();
    delay(2000);
  }
}
//____________________________C O D E 0___________________________________
void Task2code(void* pvParameters) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    Buzzer();
    serial_ESP32_Recv();
    delay(300);
  }
}

void Buzzer() {
  if (GAS_TRIGGER2 == true || myData.pm == 555) {
    tone(BUZZER, 500);
    delay(500);
    tone(BUZZER, 1000);
    delay(500);
  } else {
    tone(BUZZER, 0);
    noTone(BUZZER);
  }
}

void sensor_MQ_9() {
  sensorValue = analogRead(MQ_9);
  sensorVoltage = sensorValue / 1024 * 5.0;
  GAS__READ_VALVE2 = sensorVoltage;
  Serial.print("__________________________________________________");
  Serial.print("sensor voltage = ");
  Serial.print(sensorVoltage);
  Serial.println(" V");


  //Serial.println(" V");
}

void serial_ESP32_Sent() {
  int GAS__READ_VALVE2_SEAD = GAS__READ_VALVE2;
  String dataToSend = "#@GAS:" + String(GAS__READ_VALVE2_SEAD) + "*&";

  // ส่งข้อมูลผ่าน Serial1
  Serial1.print(dataToSend);

  // แสดงข้อมูลที่ส่งบน Serial Monitor
  Serial.print("Sent data: ");
  Serial.println(dataToSend);
}




void serial_ESP32_Recv() {
  // ตรวจสอบว่ามีข้อมูลเข้ามาที่ Serial1 หรือไม่
  if (Serial1.available()) {



    incomingData = Serial1.readString();  // ตัวอย่างข้อความที่ได้รับ
    int pmValue = 0;                      // ตัวแปรสำหรับเก็บค่า PM

    // ตรวจสอบว่าข้อความมีรูปแบบ "#@PM:" และตามด้วยตัวเลขสามหลัก
    int startIndex = incomingData.indexOf("#@PM:") + 5;     // หาตำแหน่งหลัง "#@PM:"
    int endIndex = incomingData.indexOf("&*", startIndex);  // หาตำแหน่งจบของข้อมูล

    if (startIndex != -1 && endIndex != -1) {
      String pmString = incomingData.substring(startIndex, endIndex);  // ดึงตัวเลขสามหลัก
      pmValue = pmString.toInt();
      myData.pm = pmValue;

      Serial.print("Extracted PM Value: ");
      Serial.println(pmValue);
      Serial.print("Sent myData.pm");
      Serial.println(myData.pm);
      // แปลงเป็นจำนวนเต็มและเก็บในตัวแปร

      // อัปเดตเวลาที่ได้รับข้อความล่าสุด
      lastReceivedTime = millis();
    }
  }
  // ตรวจสอบว่าครบระยะเวลาที่กำหนดหรือไม่
  if (millis() - lastReceivedTime > timeoutDuration) {
    pmValue = 0;  // รีเซ็ตค่า pmValue เป็น 0 เมื่อไม่ได้รับข้อความภายในเวลาที่กำหนด
    myData.pm = pmValue;
    forpm_trigger = 0;
    Serial.print("Extracted PM Value: ");
    Serial.println(pmValue);
    Serial.print("Sent myData.pm");
    Serial.println(myData.pm);
    Serial.println("Warning: No data received within timeout duration! Resetting pmValue to 0.");

    // อัปเดต lastReceiveTime เพื่อหลีกเลี่ยงการส่งข้อความซ้ำ
    lastReceivedTime = millis();
  }



  if (myData.pm >= 111 && myData.pm < 999) {
    forpm_trigger = 1;
  } else if (myData.pm == 999 || myData.pm == 0) {
    forpm_trigger = 0;
    // Serial.print("forpm_trigger===========");
    // Serial.println(forpm_trigger);
  }
}


void SENDER() {

  ///___________________________________SEND GAS,PM TRIGGER _______________
  boolean PM_TRIGGER;

  // เช็คค่า GAS__READ_VALVE2
  if (GAS__READ_VALVE2 >= 1 || myData.pm == 555) {
    countTrigger2++;  // เพิ่มจำนวนครั้งที่เจอเงื่อนไข

    // ถ้าเจอเงื่อนไขครบ 3 ครั้ง
    if (countTrigger2 >= 2) {
      GAS_TRIGGER2 = true;  // ตั้งค่าเป็น true
    }
  } else if (GAS__READ_VALVE2 <= 0.20) {
    if (GAS_TRIGGER2 && (millis() - lastGasResetTime2 >= 60000)) {
      GAS_TRIGGER2 = false;
      countTrigger2 = 0;
    } else if (!GAS_TRIGGER2) {
      lastGasResetTime2 = millis();  // บันทึกเวลาเริ่มใหม่
    }
  }

  Serial.print("GAS_TRIGGER2  ");
  Serial.println(GAS_TRIGGER2);
  myData.gas = (GAS_TRIGGER2);    /// ส่ง ESPNOW
  Serial.print("myData.gas = ");  /// ส่ง ESPNOW
  Serial.print(myData.gas);       /// ส่ง ESPNOW


  Serial.print("PM_TRIGGER  ");
  Serial.println(PM_TRIGGER);

  if ((GAS_TRIGGER2 + forpm_trigger) != 0) {
    myData.trigger = true;  //ส่งไป trigger
  } else {
    myData.trigger = false;
  }
  Serial.print("trigger: ");
  Serial.println(myData.trigger);
  Serial.print("===============: ");
  Serial.println(GAS_TRIGGER2 + forpm_trigger);

  ///___________________________________SEND GAS,PM TRIGGER _______________




  ///_________________________________________ NOW RECV _______________
  esp_err_t result = esp_now_send(0, (uint8_t*)&myData, sizeof(struct_message));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
  //_________________________________________ NOW RECV _______________
}