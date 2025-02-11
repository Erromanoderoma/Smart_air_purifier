/*
ECHO_SMART_AIR_V3.1 
 - ปรับการเก็บข้อมูลเพื่อคำนวนค่า AQI จาก 24h. เป็น 5minut เพื่อลดภาระของระบบ
 - ปรับ code ให้ใช้งานร่วมกับ " project Development of smart air purifiers "
 - เพิ่มระบบ ป้องกันหากอุณหภูมิ MCU ESP32 สูงเกินไปพัดลมจะทำงานเพื่อลดอุณหภูมิในระบบ  โดยอ่านค่าอุณหภูมิจากเซ็นเซอร์ภายใน ESP32
       | ESP32 มี Maximum Operating Temperature ที่ประมาณ 125°C ตามสเปกชิป (ขึ้นอยู่กับรุ่นย่อย) แต่ในความเป็นจริง การทำงานที่อุณหภูมิสูงเกิน 70-80°C
         อาจทำให้อายุการใช้งานของชิปลดลง หรือเกิดปัญหาเสถียรภาพในระยะยาวได้ เช่น รีเซ็ตตัวเองหรือการประมวลผลผิดพลาด
         ค่า 70°C เป็นการตั้งระดับความปลอดภัยที่ต่ำกว่า Maximum Temperature เพื่อให้มั่นใจว่าระบบจะทำงานได้อย่างเสถียรในระยะยาว 
         **ใช้การกรองสัญญาณรบกวน (Filtering) แบบ Low-Pass Filter หรือ Kalman Filter เพื่อปรับปรุงค่าที่อ่านให้เสถียรมากขึ้น  ใช้ 65°C ในการให้ระบบป้องกันเริ่มทำงาน
         **ใช้ sensor DH22 ประกอบการทำงานผ่านตัวแปร "temperature_sensor"

  Sketch ใช้พื้นที่จัดเก็บโปรแกรม 920,513 ไบต์ (70%) พื้นที่สูงสุดคือ 1310,720 ไบต์
*/


#include <Arduino.h>  //IoT
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Wire.h>
#include <WiFi.h>
//_____________________________________ Wi-fi Manager ______________________
#define ESP_DRD_USE_SPIFFS true
// Include Libraries
// File System Library
#include <FS.h>
// SPI Flash Syetem Library
#include <SPIFFS.h>
// WiFiManager Library
#include <WiFiManager.h>
// Arduino JSON library
#include <ArduinoJson.h>
// JSON configuration file
#define JSON_CONFIG_FILE "/test_config.json"
// Flag for saving data
bool shouldSaveConfig = false;
// Variables to hold data from custom textboxes

const char* host = "api.thingspeak.com";  // Host thingspeaks
//const char* api = "ZRGFE9DDOHFZSSCI";     //API Key
char testString[50] = "ZRGFE9DDOHFZSSCI";

// Define WiFiManager Object
WiFiManager wm;
WiFiClient client;
//_____________________________________ Wi-fi Manager ______________________


int LEVE_PM = 0;
int gasValue = 0;
bool GAS_TRIGGER = 0;

#define RXD2 17
#define TXD2 16

#define DHTPIN 5  //ความชื่น
#define DHTTYPE DHT22
#define RELAY_FAN 13  //Relay FAN
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);

boolean WIFIRESET = false;

unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

TaskHandle_t Task1;
TaskHandle_t Task2;

int storage_volume = 5;  // ปรับให้เก็บข้อมูล 60 ค่า (1 ชั่วโมงสำหรับการวัดทุก 1 นาที)
int PM_sensor2_5[5];     // เก็บค่าฝุ่นใน 1 ชั่วโมง
int i = 0;               // index ใน array
int n = 0;               // จำนวนข้อมูลที่เก็บแล้ว
int AQI = 0;
float sum = 0;  // ผลรวมของค่าฝุ่น
float sum_fate = 0;


#define INTERVAL_MESSAGE1 3000  //PM AQI + FAN
#define INTERVAL_MESSAGE2 30000
#define INTERVAL_MESSAGE3 120000  // pm aqi 2 นาที
unsigned long time_1 = 0;         //ประกาศตัวแปรเป็น global
unsigned long time_2 = 0;
unsigned long time_3 = 0;

const byte BUTTON_MENU = 26;
const byte BUTTON_LEFT = 25;
const byte BUTTON_RIGHT = 33;
const byte BUTTON_SELECT = 32;
const byte BUTTON_CANCAL = 27;
const byte BUTTON_RESET = 19;
const byte BUTTON_LCD = 18;

/////// LED POWER
#define LED_PIN 23
/////// RGB
#define RED_PIN 15
#define GREEN_PIN 2
#define BLUE_PIN 4

/////// FAN
const int FANPIN_CONTROL = 14;   /// FAN1
const int FANPIN_CONTROL2 = 12;  /// FAN2

int FAN_on_off;  // บันทึก FAN
int count = 0;
//_____ switch menu _______
int point_PM2_5 = 20;
byte MANUAL_AUTO = 0;                       /// เก็บค่าการเลือก mode
int Speed = 0;                              // 0 = low 1= mid 2= high
int SPEED_LV[] = { 50, 60, 80, 126, 255 };  /// ความเร็ว เก็บใน array
byte Mode_manual = 0;                       //// 0 = OFF  1 = ON
//int rpm; // READ RPM
boolean Arduin_ON_OFF = false;

//______________________________ LCD ^3 ______________
byte customChar[8] = {
  0b11000,
  0b01000,
  0b11000,
  0b01000,
  0b11000,
  0b00000,
  0b00000,
  0b00000
};
//______________________________ LCD ^3 ______________


/// ____________________________ แก้ไข Temperature ESP32 ใหเ stable ________
float alpha = 0.1;  // ค่า 0.0 - 1.0 ยิ่งต่ำ การกรองยิ่งแรง
float filteredValue = 0;

bool temperature_sensor = 0;  /// ใช้อิง sensorDHT 22
                              /// ____________________________ แก้ไข Temperature ESP32 ใหเ stable ________

void print_time(unsigned long time_millis);



//_____________________________________ Wi-fi Manager ______________________
void saveConfigFile()
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));

  // Create a JSON document
  StaticJsonDocument<512> json;
  json["testString"] = testString;

  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    // Error, file did not open
    Serial.println("failed to open config file for writing");
  }

  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) {
    // Error writing file
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}

bool loadConfigFile()
// Load existing configuration file
{
  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE)) {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile) {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("Parsing JSON");

          strcpy(testString, json["testString"]);

          return true;
        } else {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  } else {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }

  return false;
}


void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager* myWiFiManager)
// Called when config mode launched
{
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}
//_____________________________________ Wi-fi Manager ______________________



void setup() {
  Serial.begin(115200);


  while (!Serial)
    ;
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  lcd.init();  ///LCD
  lcd.backlight();
  dht.begin();  //ความชื่น

  ///// __LED POWER __  /////
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 100);
  ///// ____ RGB ____  /////
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(BUTTON_MENU, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_CANCAL, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);
  pinMode(BUTTON_LCD, INPUT_PULLUP);

  ///// ____ FAN ____  /////
  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_FAN, HIGH);

  pinMode(FANPIN_CONTROL, OUTPUT);
  analogWrite(FANPIN_CONTROL, 0);

  pinMode(FANPIN_CONTROL2, OUTPUT);
  analogWrite(FANPIN_CONTROL2, 0);


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


  //________WIFI_____
  WIFI_CONNECT();
}

void WIFI_CONNECT() {
  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  // Custom elements

  // Text box (String) - 50 characters maximum
  WiFiManagerParameter custom_text_box("key_API", "Thingspeak API key", testString, 50);


  // Add all defined parameters
  wm.addParameter(&custom_text_box);

  if (forceConfig)
  // Run if we need a configuration
  {
    if (!wm.startConfigPortal("Smart air purifier", "123456789")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  } else {
    if (!wm.autoConnect("Smart air purifier", "123456789")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }

  // If we get here, we are connected to the WiFi

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Lets deal with the user config values

  // Copy the string value
  strncpy(testString, custom_text_box.getValue(), sizeof(testString));
  Serial.print("testString: ");
  Serial.println(testString);

  //Convert the number value
  /*testNumber = atoi(custom_text_box_num.getValue());
  Serial.print("testNumber: ");
  Serial.println(testNumber);
  */


  // Save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfigFile();
  }
}

void loop() {
  delay(1);
}

void Task1code(void* pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;) {
    if (Arduin_ON_OFF == false) {
      Sensor();   ///================ Sensor ================
      LCDshow();  ///================ LCDShow ===============
      delay(3000);
      Serial.println(FAN_on_off);
      //_______________________________________________________________
      if (millis() - time_1 > INTERVAL_MESSAGE1) {
        time_1 = millis();
        Serial.println("=================    FAN      =============");

        serial_ESP32_Recv();
        Serial.println("=================serial_ESP32_Recv();=============");

        print_time(time_1);
        FAN_AUTO();
      }  //____________________________________________________________
    }
    SWITCH();  ///================ Switch  ===============
  }
}

void Task2code(void* pvParameters) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    if (Arduin_ON_OFF == false) {
      //_______________________________________________________________
      if (millis() - time_3 > INTERVAL_MESSAGE3) {
        time_3 = millis();
        Serial.println("=================AQI_calculate=============");
        print_time(time_3);
        AQI_calculate(pm2_5);
        RGB();  ///================ RGB ================
      }         //____________________________________________________________

      Serial.println("=================      WIFI       =============");
      WIFI();
    } else {
      Serial.println("=================   Power off     =============");
      delay(5000);
    }
  }
}

void SWITCH() {
  /////////// Switch ///////////
  static boolean ButtonMenu = false;
  static boolean ButtonReset = false;
  static boolean ButtonLCD = false;
  static boolean LCD = false;
  static boolean ESP = false;


  //_____________________ MENU  _____________
  if (digitalRead(BUTTON_MENU) == 0) {
    delay(1);
    if (digitalRead(BUTTON_MENU) == 0) {
      if (ButtonMenu == false) {
        ButtonMenu = true;
        Serial.print("SETING");
        lcd.clear();   ///////**
        SelectMenu();  /// เข้าหน้า MENU
      }
    }

  } else {
    ButtonMenu = false;
  }

  //_____________________ reset  _____________
  if (digitalRead(BUTTON_RESET) == 0) {
    analogWrite(LED_PIN, 0);
    delay(1);
    if (digitalRead(BUTTON_RESET) == 0) {
      if (ButtonReset == false) {
        ButtonReset = true;
        Serial.print("_____RESET____");
        if (LCD == false) {
          LCD = true;
          Serial.println("_____ESP OFF____");
          digitalWrite(LED_PIN, 0);
          lcd.noDisplay();
          lcd.noBacklight();
          delay(5000);
          Arduin_ON_OFF = true;
          delay(100);
          digitalWrite(RELAY_FAN, HIGH);
          analogWrite(RED_PIN, 0);
          analogWrite(GREEN_PIN, 0);
          analogWrite(BLUE_PIN, 0);
        } else {
          LCD = false;
          Serial.println("_____ESP ON____");
          lcd.display();
          lcd.backlight();
          analogWrite(LED_PIN, 255);
          delay(5000);
          Arduin_ON_OFF = false;
        }

        //   ESP.restart();
      }
    }

  } else {
    ButtonReset = false;
  }
  //_____________________   LCD   _____________
  if (digitalRead(BUTTON_LCD) == 0) {
    delay(1);
    if (digitalRead(BUTTON_LCD) == 0) {
      if (ButtonLCD == false) {
        ButtonLCD = true;
        Serial.print("_____LCD____");
        if (LCD == false) {
          LCD = true;
          Serial.println("_____LCD OFF____");
          lcd.noDisplay();    // ปิดการแสดงตัวอักษร
          lcd.noBacklight();  // ปิดไฟแบล็กไลค์
        } else {
          LCD = false;
          Serial.println("_____LCD ON____");
          lcd.display();
          lcd.backlight();
        }
      }
    }

  } else {
    ButtonLCD = false;
  }

  //__________________________________
  if (Arduin_ON_OFF == true) {
    delay(300);
  }  //_________________________________
}
void WIFI() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(500);
    return;
  }




  // Use WiFiClient class to create TCP connections
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }


  // We now create a URI for the request

  String url = "/update?api_key=";
  url += testString;
  url += "&field1=";
  url += t;
  url += "&field2=";
  url += h;
  url += "&field3=";
  url += pm1;
  url += "&field4=";
  url += pm2_5;
  url += "&field5=";
  url += pm10;
  url += "&field6=";
  url += FAN_on_off;
  url += "&field7=";
  url += gasValue;

  // ส่งข้อมูล https://api.thingspeak.com/update?api_key=RRHS37ETW76RFAWB&field1=(อุณหภูมิ)&field2=(ความชื่น)

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("closing connection");
  delay(30000);
}


void serial_ESP32_Recv() {
  ///________________________________________________ ESP32 SEAD, Recv_______________
  gasValue = 0;
  if (Serial.available()) {
    String incomingData = Serial.readString();  // ตัวอย่างข้อความที่ได้รับ

    // ตรวจสอบว่าข้อความมีรูปแบบ "#@GAS:" และตามด้วยตัวเลขสามหลัก
    int startIndex = incomingData.indexOf("#@GAS:") + 6;    // หาตำแหน่งหลัง "#@GAS:"
    int endIndex = incomingData.indexOf("*&", startIndex);  // หาตำแหน่งจบของข้อมูล

    if (startIndex != -1 && endIndex != -1) {
      String gasString = incomingData.substring(startIndex, endIndex);  // ดึงตัวเลขสามหลัก
      gasValue = gasString.toInt();
      if (gasValue >= 1) {
        GAS_TRIGGER = true;  // แก้ไขเป็น "="
      } else if (gasValue <= 0) {
        if (GAS_TRIGGER == true) {
          delay(60000);  //// หน่วงเวลา GAS ลด
          lcd.clear();
        }
        GAS_TRIGGER = false;
      }
    }
  } else {
    gasValue = 0;
  }
  Serial.print("Extracted GAS Value: ");
  Serial.println(gasValue);


  ///________________________________________________ ESP32 SEAD, Recv_______________
  if (GAS_TRIGGER == true) {
    // Use WiFiClient class to create TCP connections
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    String url = "/update?api_key=";
    url += "&field7=";
    url += gasValue;
    Serial.print("Requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
    delay(10);
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  }
}


void AQI_calculate(unsigned int PM_data) {

  int average;

  if ((PM_data < 50000) && (PM_data > 1)) {

    if (i <= storage_volume) {

      PM_sensor2_5[i] = pm2_5;  /// บันทึกข้อมูลใน array
      i++;
      n++;  /// ใช้นับรอบ
    }

    if (i > storage_volume) {  /// รีเซ็ต array เพื่อเรื่มรับใหม่
      i = 0;
    }
    sum = 0;

    // Calculate the sum of the measured values
    for (int K = 0; K < storage_volume; K++) {  /// sum +  ข้อมูลทั้งหมดใน array
      sum += PM_sensor2_5[K];
    }

    if (n <= storage_volume) {  //// ถ้ายังไม่ถึงจำนวนข้อมูลที่ต้อวการ (n)
      average = sum / n;
    }
    if (n > storage_volume) {  //// ถ้าถึงข้อมูลที่ต้องการแล้ว
      average = sum / storage_volume;

      n = storage_volume + 1;
    }
    /*
    Serial.print("PM_average = ");
    Serial.println(average);
    Serial.print("i== = ");
    Serial.println(i);
    Serial.print("n =จานวนข้อมูล= ");
    Serial.println(n);
    Serial.print("PM_sensor2_5[0] ==;");
    Serial.println(PM_sensor2_5[0]);
    Serial.print("Sum = ");
    Serial.println(sum);
*/


    float I_MAX;
    float I_MIN;
    float C_MAX;
    float C_MIN;
    float C_I = average;  //// ค่าเฉลี่ย

    if (C_I >= 0, C_I <= 25) {
      I_MIN = 0;
      I_MAX = 25;
      C_MIN = 0;
      C_MAX = 25;
    } else if (C_I >= 26, C_I <= 37) {
      I_MIN = 26;
      I_MAX = 50;
      C_MIN = 26;
      C_MAX = 37;
    } else if (C_I >= 38, C_I <= 50) {
      I_MIN = 51;
      I_MAX = 100;
      C_MIN = 38;
      C_MAX = 50;
    } else if (C_I >= 51, C_I <= 90) {
      I_MIN = 101;
      I_MAX = 200;
      C_MIN = 51;
      C_MAX = 90;
    } else if (C_I >= 91) {  //// AQI 200 ขึ้นไปคำนวณไม่ได้
      I_MIN = 0;
      I_MAX = 201;
      C_MIN = 0;
      C_MAX = 91;
    }


    float M = (C_I - C_MIN);
    float Z = ((I_MAX - I_MIN) / (C_MAX - C_MIN) * M);
    float G = Z + I_MIN;
    AQI = round(G);  // ปัดเศษค่า AQI

    Serial.print("PM2.5 Average (1h): ");
    Serial.println(average);
    Serial.print("AQI: ");
    Serial.println(AQI);
  }
}



void Sensor() {

  int index = 0;       /// pm
  char value;          /// pm
  char previousValue;  /// pm
  ++value;

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.println(" *F\t");

  /// ____________________________ Temperature ป้องกันระบบ ________
  if (t >= 30) {
    temperature_sensor = true;
  } else if (t <= 27) {
    temperature_sensor = false;
  }
  /// ____________________________ Temperature ป้องกันระบบ ________

  while (Serial2.available()) {
    value = Serial2.read();
    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d)) {
      Serial.println("Cannot find the data header.");
      break;
    }

    if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
      previousValue = value;
    } else if (index == 5) {
      pm1 = 256 * previousValue + value;
      Serial.print("{ ");
      Serial.print("\"pm1\": ");
      Serial.print(pm1);
      Serial.print(" ug/m3");
      Serial.print(", ");
    } else if (index == 7) {
      pm2_5 = 256 * previousValue + value;
      Serial.print("\"pm2_5\": ");
      Serial.print(pm2_5);
      Serial.print(" ug/m3");
      Serial.print(", ");
    } else if (index == 9) {
      pm10 = 256 * previousValue + value;
      Serial.print("\"pm10\": ");
      Serial.print(pm10);
      Serial.print(" ug/m3");
    } else if (index > 15) {
      break;
    }
    index++;
  }
  while (Serial2.available()) Serial2.read();
  Serial.println(" }");
}



void LCDshow() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }


  int SET1 = 0;
  int SET2 = 0;

  if (pm10 >= 100) {
    SET1 = 1;
    SET2 = 0;
  }
  if (pm10 >= 1000) {
    SET1 = 2;
    SET2 = 1;
  }
  if (pm10 >= 10000) {
    SET1 = 3;
    SET2 = 2;
  }

  if (GAS_TRIGGER == false) {
    if (pm1 > 0) {

      lcd.createChar(0, customChar);  // ยกกำลัง 3


      lcd.setCursor(17, 0);
      lcd.print("[");
      if (MANUAL_AUTO == 0) {
        lcd.setCursor(18, 0);
        lcd.print("A");
      }
      if (MANUAL_AUTO == 1) {
        lcd.setCursor(18, 0);
        lcd.print("M");
      }
      lcd.setCursor(19, 0);
      lcd.print("]");


      if (AQI < 200) {
        lcd.setCursor(5, 0);
        lcd.print("AQI =");
        lcd.setCursor(10, 0);
        lcd.print(AQI);
        lcd.setCursor(13, 0);
        lcd.print("   ");
        if ((AQI < 99) && (AQI >= 10)) {
          lcd.setCursor(12, 0);
          lcd.print("  ");
        } else if (AQI < 10) {
          lcd.setCursor(11, 0);
          lcd.print("  ");
        }
      } else if (AQI >= 200) {
        lcd.setCursor(4, 0);
        lcd.print("AQI=");
        lcd.setCursor(9, 0);
        lcd.print("over200");
      }



      lcd.setCursor(14 + SET1, 1);
      lcd.print(t);
      lcd.setCursor(19 + SET2, 1);
      lcd.print("C");
      lcd.setCursor(14 + SET1, 2);
      lcd.print(h);
      lcd.setCursor(19 + SET2, 2);
      lcd.print("%");

      ///////////////////////////////// PM1
      lcd.setCursor(0, 1);
      lcd.print("PM1  :");
      lcd.setCursor(6, 1);
      lcd.print(pm1);
      lcd.setCursor(8 + SET1, 1);
      lcd.print("ug/m");
      lcd.setCursor(12 + SET1, 1);
      lcd.write((uint8_t)0);
      if ((pm1 < 100) && (pm1 >= 10)) {
        lcd.setCursor(13 + SET1, 1);
        lcd.print(" ");  //LCD --
        if (pm10 >= 100) {
          lcd.setCursor(8, 1);
          lcd.print(" ");  //LCD --
        }
      } else if (pm1 < 10) {
        lcd.setCursor(7, 1);
        lcd.print(" ");  //LCD --
      }

      ///////////////////////////////// PM2.5
      lcd.setCursor(0, 2);
      lcd.print("PM2.5:");
      lcd.setCursor(6, 2);
      lcd.print(pm2_5);
      lcd.setCursor(8 + SET1, 2);
      lcd.print("ug/m");
      lcd.setCursor(12 + SET1, 2);
      lcd.write((uint8_t)0);
      if ((pm2_5 < 100) && (pm2_5 >= 10)) {
        lcd.setCursor(13 + SET1, 2);
        lcd.print(" ");  //LCD --
        if (pm10 >= 100) {
          lcd.setCursor(8, 2);
          lcd.print(" ");  //LCD --
        }
      } else if (pm2_5 < 10) {
        lcd.setCursor(7, 2);
        lcd.print(" ");  //LCD --
      }
      ///////////////////////////////// PM10
      lcd.setCursor(0, 3);
      lcd.print("PM10 :");
      lcd.setCursor(6, 3);
      lcd.print(pm10);
      lcd.setCursor(8 + SET1, 3);
      lcd.print("ug/m  ");
      lcd.setCursor(12 + SET1, 3);
      lcd.write((uint8_t)0);
      if ((pm10 < 100) && (pm10 >= 10)) {
        lcd.setCursor(13 + SET1, 3);
        lcd.print(" ");
        if (pm10 >= 100) {
          lcd.setCursor(8, 3);
          lcd.print(" ");
        }
      } else if (pm10 < 10) {
        lcd.setCursor(7, 3);
        lcd.print(" ");
        //LCD --
      }
      /*lcd.setCursor(14 + SET1, 3);
      lcd.print("GAS=N"); */
    }

    ///________________________________________________ ESP32 SEAD, Recv_______________
    // สร้างข้อความในรูปแบบ #@PM:<pmValue>#@
    String dataToSend = "#@PM:" + String(LEVE_PM) + "&*";

    // ส่งข้อมูลผ่าน Serial1
    Serial.print(dataToSend);

    // แสดงข้อมูลที่ส่งบน Serial Monitor
    Serial.print("Sent data: ");
    Serial.println(dataToSend);
    ///________________________________________________ ESP32 SEAD, Recv_______________
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("!!! GAS DETECTED !!!");

    lcd.setCursor(2, 1);
    lcd.print("High Gas Level!");

    lcd.setCursor(2, 2);
    lcd.print("Fan is Activated");

    lcd.setCursor(1, 3);
    lcd.print("Check Surroundings");
  }
}


/////////==================== RGB =====================
void RGB() {

  //9 R
  //10G
  //11B
  if (AQI <= 25) {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 255);
  } else if ((AQI >= 26) && (AQI <= 50)) {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 255);
    analogWrite(BLUE_PIN, 0);
  } else if ((AQI >= 51) && (AQI <= 100)) {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 255);
    analogWrite(BLUE_PIN, 0);
  } else if ((AQI >= 101) && (AQI < 200)) {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 165);
    analogWrite(BLUE_PIN, 0);
  } else if (AQI >= 200) {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
  }
}



/////////==================== FAN =====================
void FAN() {
  if (MANUAL_AUTO == 1) {    /// MANUAL ***
    if (Mode_manual == 0) {  //// Manual ON/OFF
      digitalWrite(RELAY_FAN, HIGH);
      delay(300);
      analogWrite(FANPIN_CONTROL, 0);
      analogWrite(FANPIN_CONTROL2, 0);
      LEVE_PM = 999;
      FAN_on_off = 0;
      /*lcd.setCursor(13, 1);  ///LCD
      lcd.print("FAN=OFF"); */
    } else if (Mode_manual == 1) {
      digitalWrite(RELAY_FAN, LOW);
      delay(300);
      analogWrite(FANPIN_CONTROL, SPEED_LV[Speed]);
      analogWrite(FANPIN_CONTROL2, SPEED_LV[Speed]);
      /* lcd.setCursor(14, 1);  ///LCD
      lcd.print("FAN=ON"); */

      FAN_on_off = ((Speed + 1) - ((Speed + 1) * 2));  // บันทึกค่าให้ mode manual ติด -  ค่ายิ่งน้อยความเป็นรอบยิ่งสูง
    }
  }
}

void FAN_AUTO() {
  if (MANUAL_AUTO == 0) {  // AUTO ***
    Serial.println("AUTO");

    if (GAS_TRIGGER == false) {
      // การควบคุมพัดลมตามค่า pm2.5
      if (pm2_5 <= 24) {


        // อ่านค่าอุณหภูมิจากเซ็นเซอร์ภายใน ESP32 ______________________________________________
        float currentReading = temperatureRead();  // อ่านอุณหภูมิของ MCU
        filteredValue = alpha * currentReading + (1 - alpha) * filteredValue;

        if (isnan(filteredValue)) {  // หากเกิดข้อผิดพลาดในการอ่านค่า
          Serial.println("Failed to read internal temperature sensor!");
          digitalWrite(RELAY_FAN, LOW);     // เปิดพัดลมเพื่อป้องกันความร้อน
          analogWrite(FANPIN_CONTROL, 25);  // ตั้งความเร็วพัดลมเป็นระดับกลาง
          analogWrite(FANPIN_CONTROL2, 25);
          delay(500);
          return;
        }


        Serial.print("ESP32-Temperature filter =");
        Serial.println(filteredValue);
        Serial.print("ESP32-Temperature");
        Serial.println(currentReading);
        Serial.print("Tempratuer_SENSOR ==");
        Serial.println(temperature_sensor);
        // อ่านค่าอุณหภูมิจากเซ็นเซอร์ภายใน ESP32 ______________________________________________

        if (filteredValue >= 63 || temperature_sensor == true) {
          Serial.println("+++ !!! อุณหภูมิ MCU สูง ระบบป้องกันทำงาน !!! +++");
          digitalWrite(RELAY_FAN, LOW);
          delay(300);
          analogWrite(FANPIN_CONTROL, 25);
          analogWrite(FANPIN_CONTROL2, 25);
          FAN_on_off = 1;
        } else if (filteredValue <= 55) {
          digitalWrite(RELAY_FAN, HIGH);
          delay(300);
          analogWrite(FANPIN_CONTROL, 0);
          analogWrite(FANPIN_CONTROL2, 0);
          LEVE_PM = 999;
          FAN_on_off = 0;
        }

      } else {
        digitalWrite(RELAY_FAN, LOW);
        delay(300);
        if ((pm2_5 >= 26) && (pm2_5 <= 37)) {
          analogWrite(FANPIN_CONTROL, 50);
          analogWrite(FANPIN_CONTROL2, 50);
          LEVE_PM = 111;
          FAN_on_off = 1;
        } else if ((pm2_5 >= 38) && (pm2_5 <= 50)) {
          analogWrite(FANPIN_CONTROL, 60);
          analogWrite(FANPIN_CONTROL2, 60);
          LEVE_PM = 222;
          FAN_on_off = 2;
        } else if ((pm2_5 >= 51) && (pm2_5 <= 91)) {
          analogWrite(FANPIN_CONTROL, 80);
          analogWrite(FANPIN_CONTROL2, 80);
          LEVE_PM = 333;
          FAN_on_off = 3;
        } else if (pm2_5 >= 91) {
          analogWrite(FANPIN_CONTROL, 126);
          analogWrite(FANPIN_CONTROL2, 126);
          LEVE_PM = 444;
          FAN_on_off = 4;
        } else if (pm2_5 >= 1000) {
          analogWrite(FANPIN_CONTROL, 226);
          analogWrite(FANPIN_CONTROL2, 226);
          LEVE_PM = 555;  /// แจ้งเตือน buzzer
          FAN_on_off = 4;
        }
      }
    } else {  // เมื่อค่า GAS_TRIGGER เป็น true
      digitalWrite(RELAY_FAN, LOW);
      analogWrite(FANPIN_CONTROL, 160);  // เปิดพัดลมที่ความเร็วต่ำ
      analogWrite(FANPIN_CONTROL2, 160);
      //  LEVE_PM = 111;  // ระดับ PM สำหรับแสดงสถานะ
      FAN_on_off = 1;
    }
  }
}



/////////////////////////////////////////// S W I T C H //////////////////////////////////////////////
void SelectMenu(void) {
  boolean Display = true;
  boolean Exit = false;
  boolean ButtonMenu = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;

  lcd.display();
  lcd.backlight();


  byte Menu = 0;
  const char MenuText[4][21] = { "1: MODE            ",
                                 "2: SET POINT       ",
                                 "3: NETWORK         ",
                                 "4: ABOUT           " };

  while (Exit == false) {

    if (Display == true) {
      Display = false;
      lcd.setCursor(0, 0);
      lcd.print("    Select Menu     ");
      lcd.setCursor(0, 1);
      lcd.print(MenuText[Menu]);
    }

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (ButtonMenu == false) {

          ButtonMenu = true;
          switch (Menu) {  //// ใส่ฟังชั่นที่นี้

            case 0:
              Serial.println("Yur select [MODE]");
              MODE();
              break;

            case 1:
              Serial.println("You select [SET Point]");
              Select_POINT();
              break;

            case 2:
              Serial.println("You select [WiFi Seting]");
              NETWORK();
              break;


            case 3:
              Serial.println("You select [ABOUT]");
              ABOUT();
              break;
          }
        }
      }
    } else {
      ButtonMenu = false;
    }


    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (Menu > 0) {
            Menu--;

          } else {
            Menu = 3;
          }
          Display = true;
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (Menu < 3) {
            Menu++;

          } else {
            Menu = 0;
          }
          Display = true;
        }
      }
      ButtonRight = false;
    }


    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;
          Exit = true;
          lcd.clear();  //// ล้างหน้าจอ LCD ก่อน Exit
        }
      }
    } else {
      ButtonCanCel = false;
    }
    delay(100);  // ป้องกันบัค
  }
}

void Select_POINT(void) {  /// point PM2.5

  boolean select_point = true;
  boolean Exit_point = false;
  boolean ButtonCanCel = false;

  while (Exit_point == false) {


    lcd.setCursor(0, 2);
    lcd.print("PM2.5 POINT SETTING");  /// ตัวการทำงารของ FAN
    lcd.setCursor(7, 3);
    lcd.print("<>");
    lcd.setCursor(9, 3);
    lcd.print(point_PM2_5);




    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        point_PM2_5--;
      }
    }

    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        point_PM2_5++;
      }
    }

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(1);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (select_point == false) {
          select_point = true;
          Serial.print("SELECT");

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");


          Exit_point = true;
        }
      }
    } else {
      select_point = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          Serial.print("CANCEL");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit_point = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}


void MODE(void) {

  boolean ButtonSelect = true;
  boolean Exit_MODE = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;


  byte mode = 0;
  const char ModeText[4][21] = { "auto  ",
                                 "manual" };


  while (Exit_MODE == false) {


    lcd.setCursor(5, 3);
    lcd.print("<>");
    lcd.setCursor(8, 3);
    lcd.print(ModeText[mode]);

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (ButtonSelect == false) {
          ButtonSelect = true;
          switch (mode) {  //// ใส่ฟังชั่นที่นี้

            case 0:
              Serial.println("You select [AUTO]");
              MODE_AUTO();
              break;

            case 1:
              Serial.println("Yur select [MANUAL]");

              if (MANUAL_AUTO == 0) {  ////// เช็คถ้าหากเป็น mode auto มาก่อนจะกำหนด PWM = 0
                analogWrite(FANPIN_CONTROL, 0);
                analogWrite(FANPIN_CONTROL2, 0);
                digitalWrite(RELAY_FAN, HIGH);
                delay(300);
              }

              MODE_MANUAL();
              break;
          }

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");


          Exit_MODE = true;
        }
      }
    } else {
      ButtonSelect = false;
    }



    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (mode > 0) {
            mode--;

          } else {
            mode = 1;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (mode < 1) {
            mode++;

          } else {
            mode = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          Serial.print("CANCEL");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit_MODE = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}

void MODE_AUTO(void) {
  boolean select_auto = true;
  boolean Exit_auto = false;




  ////// mode = เอาไปปสั้งงาน void FAN

  while (Exit_auto == false) {

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (select_auto == false) {
          select_auto = true;
          Serial.print("cancel");

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");

          MANUAL_AUTO = 0;  /// บันทึกการตั้งค่า MODE

          Exit_auto = true;
        }
      }
    } else {
      select_auto = false;
    }
  }
  FAN_AUTO();
}

void MODE_MANUAL(void) {

  boolean ButtonSelect = true;
  boolean Exit_MODE = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;
  byte Manual_select = 0;

  const char Manual[4][21] = { "ON/OFF",
                               "SPEED " };


  while (Exit_MODE == false) {


    lcd.setCursor(5, 3);
    lcd.print("<>");
    lcd.setCursor(8, 3);
    lcd.print(Manual[Manual_select]);

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (ButtonSelect == false) {
          ButtonSelect = true;
          switch (Manual_select) {  //// ใส่ฟังชั่นที่นี้

            case 0:
              Serial.println("You select [AUTO]");
              MANUAL_IO();
              break;

            case 1:
              Serial.println("Yur select [MANUAL]");
              FAN_SPEED();
              break;
          }
          Serial.print("cancel");

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");


          Exit_MODE = true;
        }
      }
    } else {
      ButtonSelect = false;
    }



    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (Manual_select > 0) {
            Manual_select--;

          } else {
            Manual_select = 1;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (Manual_select < 1) {
            Manual_select++;

          } else {
            Manual_select = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          Serial.print("CANCEL");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit_MODE = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}

void MANUAL_IO(void) {  ////ให้เลือก mode พัดลม Auto ,manual
  boolean select = true;
  boolean Exit = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;

  if (Mode_manual == 0) {
    lcd.setCursor(14, 2);
    lcd.print("OFF");
  } else {
    lcd.setCursor(14, 2);
    lcd.print("ON");
  }


  const char Show_mode_manual[4][21] = { "OFF   ",
                                         "ON    " };



  ////// mode = เอาไปปสั้งงาน void FAN

  while (Exit == false) {

    switch (Mode_manual) {  //// ใส่ฟังชั่นที่นี้

      case 0:
        lcd.setCursor(0, 2);
        lcd.print("mode manual = ");


        lcd.setCursor(7, 3);
        lcd.print("<>");
        lcd.setCursor(9, 3);
        lcd.print("OFF    ");
        break;

      case 1:
        lcd.setCursor(0, 2);
        lcd.print("mode manual = ");


        lcd.setCursor(7, 3);
        lcd.print("<>");
        lcd.setCursor(9, 3);
        lcd.print("ON    ");
        break;
    }


    /*
    lcd.setCursor(0, 2);
    lcd.print("mode manual = ");
    lcd.setCursor(14, 2);
    lcd.print(Show_mode_manual[Mode_manual]);

    lcd.setCursor(7, 3);
    lcd.print("<>");
    lcd.setCursor(9, 3);
    lcd.print(Show_mode_manual[Mode_manual]);  */

    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (Mode_manual > 0) {
            Mode_manual--;

          } else {
            Mode_manual = 1;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (Mode_manual < 1) {
            Mode_manual++;

          } else {
            Mode_manual = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (select == false) {
          select = true;
          Serial.print("cancel");

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");

          MANUAL_AUTO = 1;  /// บันทึกการตั้งค่า


          Exit = true;
        }
      }
    } else {
      select = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          Serial.print("CANCEL");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
  FAN();
}


void FAN_SPEED(void) {  ///// ยังไม่ใช้
  boolean select = true;
  boolean Exit = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;

  while (Exit == false) {


    lcd.setCursor(0, 2);
    lcd.print("FAN SPEED SETTING");  /// ตัวการทำงารของ FAN
    lcd.setCursor(7, 3);
    lcd.print("<>");
    lcd.setCursor(9, 3);
    lcd.print(Speed + 1);  /// ความเร็วพัดลม
    lcd.setCursor(10, 3);
    lcd.print("   ");  /// ความเร็วพัดลม




    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (Speed > 0) {
            Speed--;

          } else {
            Speed = 4;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (Speed < 4) {
            Speed++;

          } else {
            Speed = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (select == false) {
          select = true;
          Serial.print("cancel");

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");

          MANUAL_AUTO = 1;  /// บันทึกการตั้งค่า MODE

          Exit = true;
        }
      }
    } else {
      select = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          Serial.print("CANCEL");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
  FAN();
}

void ABOUT() {
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean select = true;
  boolean Exit = false;
  boolean ButtonCanCel = false;
  int about = 0;

  lcd.clear();
  delay(10);


  while (Exit == false) {
    lcd.setCursor(7, 0);
    lcd.print("ABOUT");

    if (about == 0) {
      lcd.setCursor(0, 1);
      lcd.print("Project     ");
      lcd.setCursor(0, 2);
      lcd.print("Smart air purifier");
      lcd.setCursor(0, 3);
      lcd.print("                   ");
    } else if (about == 1) {
      lcd.setCursor(0, 1);
      lcd.print("Organizer");
      lcd.setCursor(0, 2);
      lcd.print("Benjamin Ueakit");
      lcd.setCursor(0, 3);
      lcd.print("Somyot Channamsai");
    } else if (about == 2) {
      lcd.setCursor(0, 1);
      lcd.print("    Thingspeak");
      lcd.setCursor(0, 2);
      lcd.print("[https://thingspeak.");
      lcd.setCursor(0, 3);
      lcd.print("com/channels/1976729");
    } else if (about == 3) {
      lcd.setCursor(0, 1);
      lcd.print("      Roi-Et");
      lcd.setCursor(0, 2);
      lcd.print(" Technical College");
      lcd.setCursor(7, 3);
      lcd.print("2022");
    }


    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (about > 0) {
            about--;
            lcd.clear();
            delay(10);

          } else {
            about = 3;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (about < 3) {
            about++;
            lcd.clear();
            delay(10);

          } else {
            about = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (select == false) {
          select = true;
          lcd.clear();
          delay(10);

          lcd.setCursor(0, 0);
          lcd.print("    Select Menu     ");
          lcd.setCursor(0, 1);
          lcd.print("4: ABOUT");

          Exit = true;
        }
      }
    } else {
      select = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          lcd.clear();
          delay(10);

          lcd.setCursor(0, 0);
          lcd.print("    Select Menu     ");
          lcd.setCursor(0, 1);
          lcd.print("4: ABOUT");

          Exit = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}
//NETWORK()

void NETWORK() {

  boolean ButtonSelect = true;
  boolean Exit = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;
  byte Manual_select = 0;

  const char Manual[4][21] = { "RESET NETWORK  ",
                               "NETWORK MANAGER" };


  while (Exit == false) {


    lcd.setCursor(2, 3);
    lcd.print("<>");
    lcd.setCursor(5, 3);
    lcd.print(Manual[Manual_select]);

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (ButtonSelect == false) {
          ButtonSelect = true;
          switch (Manual_select) {  //// ใส่ฟังชั่นที่นี้

            case 0:
              Serial.println("You select [RESET NETWORK]");
              RESET_NETWORK();
              break;

            case 1:
              Serial.println("Yur select [WIFI]");
              NETWORK_MANNAGER();
              break;
          }
          Serial.print("cancel");

          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");


          Exit = true;
        }
      }
    } else {
      ButtonSelect = false;
    }



    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (Manual_select > 0) {
            Manual_select--;

          } else {
            Manual_select = 1;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (Manual_select < 1) {
            Manual_select++;

          } else {
            Manual_select = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }
    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          Serial.print("CANCEL");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}

void RESET_NETWORK() {

  boolean select = true;
  boolean Exit = false;
  boolean ButtonLeft = false;
  boolean ButtonRight = false;
  boolean ButtonCanCel = false;
  int RESET = 0;

  lcd.setCursor(0, 0);
  lcd.print("   RESET NETWORK    ");
  lcd.setCursor(0, 1);
  lcd.print("   Are you sure ?   ");

  const char Manual[4][21] = { "  [NO]        YES   ",
                               "   NO        [YES]  " };

  while (Exit == false) {


    lcd.setCursor(0, 3);
    lcd.print(Manual[RESET]);

    if (digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_SELECT) == 0) {
        if (select == false) {
          select = true;
          switch (RESET) {

            case 0:
              Exit = true;
              break;

            case 1:
              WIFIRESET = true;
              if (WIFIRESET == true) {

                lcd.setCursor(0, 0);
                lcd.print("     SETING WIFI    ");
                lcd.setCursor(0, 1);
                lcd.print("N:Smart air purifier");
                lcd.setCursor(0, 2);
                lcd.print("Password:123456789  ");
                lcd.setCursor(0, 3);
                lcd.print("IP:192.168.4.1      ");

                wm.resetSettings();
                WIFI_CONNECT();
                WIFIRESET = false;
                delay(1000);
              }
              delay(100);
              break;
          }
          lcd.setCursor(0, 0);
          lcd.print("    Select Menu     ");
          lcd.setCursor(0, 1);
          lcd.print("3: NETWORK          ");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");

          Exit = true;
        }
      }
    } else {
      select = false;
    }


    if (digitalRead(BUTTON_LEFT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_LEFT) == 0) {
        if (ButtonLeft == false) {
          ButtonLeft = true;
          if (RESET > 0) {
            RESET--;

          } else {
            RESET = 1;
          }
        }
      }
    } else {
      ButtonLeft = false;
    }


    if (digitalRead(BUTTON_RIGHT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_RIGHT) == 0) {
        if (ButtonRight == false) {
          ButtonRight = true;
          if (RESET < 1) {
            RESET++;

          } else {
            RESET = 0;
          }
        }
      }
    } else {
      ButtonRight = false;
    }

    if (digitalRead(BUTTON_CANCAL) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;
          lcd.setCursor(0, 0);
          lcd.print("    Select Menu     ");
          lcd.setCursor(0, 1);
          lcd.print("3: NETWORK          ");
          lcd.setCursor(0, 2);
          lcd.print("                    ");
          lcd.setCursor(0, 3);
          lcd.print("                    ");
          Exit = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}

void NETWORK_MANNAGER() {
  boolean Exit = false;
  boolean ButtonCanCel = false;

  lcd.clear();
  delay(10);
  lcd.setCursor(0, 0);
  lcd.print("  NETWORK MANAGER   ");

  while (Exit == false) {

    if (WiFi.status() != WL_CONNECTED) {
      lcd.setCursor(0, 1);
      lcd.print("Wifi failed !!      ");
      lcd.setCursor(0, 2);
      lcd.print(".............");
      lcd.setCursor(0, 3);
      lcd.print("Key:");
      lcd.setCursor(4, 3);
      lcd.print(testString);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Wifi Connexted.    ");
      lcd.setCursor(0, 2);
      lcd.print(WiFi.localIP());
      lcd.setCursor(0, 3);
      lcd.print("Key:");
      lcd.setCursor(4, 3);
      lcd.print(testString);
    }
    delay(300);


    if (digitalRead(BUTTON_CANCAL) == 0 || digitalRead(BUTTON_SELECT) == 0) {
      delay(100);
      if (digitalRead(BUTTON_CANCAL) == 0 || digitalRead(BUTTON_SELECT) == 0) {
        if (ButtonCanCel == false) {
          ButtonCanCel = true;

          lcd.clear();
          delay(10);

          lcd.setCursor(0, 0);
          lcd.print("    Select Menu     ");
          lcd.setCursor(0, 1);
          lcd.print("3: NETWORK          ");

          Exit = true;
        }
      }
    } else {
      ButtonCanCel = false;
    }
  }
}
void print_time(unsigned long time_millis) {
  Serial.print("Time: ");
  Serial.print(time_millis / 1000);
  Serial.print("s - ");
}