#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <ThingSpeak.h>
#include <WiFi.h>

// ======================== PIN CONFIGURATION ========================
const int WIND_SENSOR_PIN = 15;
const int DHT_PIN = 4;
const int SOIL_MOISTURE_PIN = 32;
const int AIR_VALUE = 3620;
const int WATER_VALUE = 1680;

// ======================== WiFi CREDENTIALS ========================
const char* ssid = "Lab-Fisika";
const char* password = "EAPFisika1300Teknik";

// ======================== ThingSpeak CONFIG ========================
unsigned long myChannelNumber = 2953765;
const char* myWriteAPIKey = "VYJOT83DXXEPTOES";
WiFiClient client;

// ======================== SENSOR & LCD OBJECTS ========================
DHT dht(DHT_PIN, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======================== SHARED VARIABLES ========================
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile unsigned long rotationCount = 0;
float windSpeed = 0;
float temperature = 0;
float humidity = 0;
int soilMoisture = 0;
bool dhtStatus = false;
bool anemometerStatus = false;
bool soilSensorStatus = false;

// ======================== TASK HANDLES ========================
TaskHandle_t SensorTask;
TaskHandle_t DisplayTask;

// ======================== TIMING VARIABLES ========================
unsigned long previousMillisThingspeak = 0;
const long thingspeakInterval = 20000; // 20 seconds

// ======================== INTERRUPT HANDLER ========================
void IRAM_ATTR countRotation() {
  portENTER_CRITICAL_ISR(&mux);
  rotationCount++;
  portEXIT_CRITICAL_ISR(&mux);
}

// ======================== SENSOR TASK (CORE 0) ========================
void sensorReadingTask(void *pvParameters) {
  while(1) {
    // Wind speed calculation
    static unsigned long lastWindUpdate = 0;
    if(millis() - lastWindUpdate >= 1000) {
      portENTER_CRITICAL(&mux);
      windSpeed = rotationCount * 0.111;
      rotationCount = 0;
      anemometerStatus = (windSpeed > 0.1);
      portEXIT_CRITICAL(&mux);
      lastWindUpdate = millis();
    }

    // Environmental sensors
    static unsigned long lastEnvRead = 0;
    if(millis() - lastEnvRead >= 2000) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      
      portENTER_CRITICAL(&mux);
      if(!isnan(h) && !isnan(t)) {
        humidity = h;
        temperature = t;
        dhtStatus = true;
      } else {
        dhtStatus = false;
      }
      portEXIT_CRITICAL(&mux);
      lastEnvRead = millis();
    }

    // Soil moisture reading
    int rawValue = analogRead(SOIL_MOISTURE_PIN);
    portENTER_CRITICAL(&mux);
    soilMoisture = constrain(map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100), 0, 100);
    soilSensorStatus = (soilMoisture >= 0 && soilMoisture <= 100);
    portEXIT_CRITICAL(&mux);
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ======================== DISPLAY TASK (CORE 1) ========================
void displayUpdateTask(void *pvParameters) {
  lcd.init();
  lcd.backlight();
  lcd.clear();

  unsigned long lastScreenSwitch = 0;
  bool currentScreen = 0;

  while(1) {
    if(millis() - lastScreenSwitch >= 2000) {
      currentScreen = !currentScreen;
      lastScreenSwitch = millis();
      lcd.clear();
    }

    portENTER_CRITICAL(&mux);
    float currentTemp = temperature;
    float currentHum = humidity;
    float currentWind = windSpeed;
    int currentSoil = soilMoisture;
    portEXIT_CRITICAL(&mux);

    if(currentScreen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(currentTemp, 1);
      lcd.print("C ");
      
      lcd.setCursor(0, 1);
      lcd.print("Humi: ");
      lcd.print(currentHum, 1);
      lcd.print("%");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Wind: ");
      lcd.print(currentWind, 1);
      lcd.print("m/s");
      
      lcd.setCursor(0, 1);
      lcd.print("Soil: ");
      lcd.print(currentSoil);
      lcd.print("%  ");
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// ======================== ThingSpeak UPLOAD ========================
void uploadToThingSpeak() {
  portENTER_CRITICAL(&mux);
  float currentTemp = temperature;
  float currentHum = humidity;
  float currentWind = windSpeed;
  int currentSoil = soilMoisture;
  bool currentDhtStatus = dhtStatus;
  bool currentAnemometerStatus = anemometerStatus;
  bool currentSoilStatus = soilSensorStatus;
  portEXIT_CRITICAL(&mux);

  ThingSpeak.setField(1, currentTemp);
  ThingSpeak.setField(2, currentHum);
  ThingSpeak.setField(3, currentWind);
  ThingSpeak.setField(4, currentSoil);
  ThingSpeak.setField(5, currentDhtStatus ? 1 : 0);
  ThingSpeak.setField(6, currentAnemometerStatus ? 1 : 0);
  ThingSpeak.setField(7, currentSoilStatus ? 1 : 0);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200) {
    Serial.println("ThingSpeak update successful");
  } else {
    Serial.println("ThingSpeak update failed. Error: " + String(x));
  }
}

// ======================== WiFi CONNECTION ========================
void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ======================== SETUP ========================
void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi
  connectToWiFi();
  ThingSpeak.begin(client);
  
  // Initialize sensors
  pinMode(WIND_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIND_SENSOR_PIN), countRotation, FALLING);
  dht.begin();

  // Create tasks
  xTaskCreatePinnedToCore(
    sensorReadingTask,
    "SensorTask",
    10000,
    NULL,
    1,
    &SensorTask,
    0);

  xTaskCreatePinnedToCore(
    displayUpdateTask,
    "DisplayTask",
    10000,
    NULL,
    1,
    &DisplayTask,
    1);
}

// ======================== MAIN LOOP ========================
void loop() {
  if(WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillisThingspeak >= thingspeakInterval) {
    uploadToThingSpeak();
    previousMillisThingspeak = currentMillis;
  }
  
  delay(100); // Prevent watchdog trigger
}