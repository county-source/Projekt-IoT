#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp_sleep.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>
#include <time.h>

// WiFi credentials
const char* ssid     = "TP-LINK_F964_5G";
const char* password = "1974197419";

// NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Deep sleep settings
const int wakeUpHour = 6;
const int sleepHour = 22;

// Global time variable
int fixedHour = -1;

// ESP32 Pin Definitions
#define DHTPIN 2           
#define DHTTYPE DHT11       
#define MOISTURE_PIN 35     
#define WATERLEVEL_PIN 33  

// Sensor calibration values
int dryValue = 3300;
int wetValue = 1300;
int emptyValue = 0;
int fullValue = 4095;

// OLED Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, 22, 21);  

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Function to put ESP32 into deep sleep
void goToDeepSleep(int sleepDurationSeconds) {
    Serial.printf("\nESP32 entering deep sleep for %d seconds...\n", sleepDurationSeconds);
    delay(200);
    esp_sleep_enable_timer_wakeup(sleepDurationSeconds * 1000000ULL);
    esp_deep_sleep_start();
}

// Function to get current hour
int getCurrentHour() {
    timeClient.update();
    time_t now = timeClient.getEpochTime();
    struct tm rtcTime;
    localtime_r(&now, &rtcTime);

    fixedHour = rtcTime.tm_hour;
    Serial.printf("\nCurrent time: %02d:%02d:%02d\n", rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec);
    return fixedHour;
}

// Function to read DHT11 temperature with retries
float readDHTTemperature() {
    float temp;
    int attempts = 0;

    while (attempts < 5) {
        temp = dht.readTemperature();
        if (!isnan(temp) && temp != 0) return temp;
        attempts++;
        delay(1000);
    }

    return -99;
}

// Function to read DHT11 humidity with retries
float readDHTHumidity() {
    float humidity;
    int attempts = 0;

    while (attempts < 5) {
        humidity = dht.readHumidity();
        if (!isnan(humidity) && humidity != 0) return humidity;
        attempts++;
        delay(1000);
    }

    return -99;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nStarting ESP32...");

    // Initialize OLED display
    u8g2.begin();
    u8g2.setPowerSave(0);

    // Start DHT sensor
    dht.begin();

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected!");

    // Start NTP client
    timeClient.begin();
    timeClient.update();

    int currentHour = getCurrentHour();

    // Enter deep sleep if the current time is between 22:00 - 5:59
    if (currentHour >= sleepHour || currentHour < wakeUpHour) {
        int sleepTime;
        if (currentHour >= sleepHour) {
            sleepTime = (24 - currentHour + wakeUpHour) * 3600;
        } else {
            sleepTime = (wakeUpHour - currentHour) * 3600;
        }
        Serial.printf("Sleeping until %02d:00\n", wakeUpHour);
        goToDeepSleep(sleepTime);
    }
}

void loop() {
    int soilMoisture = analogRead(MOISTURE_PIN);
    int waterLevel = analogRead(WATERLEVEL_PIN);

    float airTemp = readDHTTemperature();
    float airHumidity = readDHTHumidity();

    if (airTemp == -99) airTemp = 0;
    if (airHumidity == -99) airHumidity = 0;

    int roundedTemp = round(airTemp);
    int soilMoisturePercent = map(soilMoisture, dryValue, wetValue, 0, 100);
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    int waterThreshold = (emptyValue + fullValue) / 2;
    String waterState = (waterLevel > waterThreshold) ? "Full" : "Empty";

    // Print sensor data to Serial Monitor
    Serial.println("\n----------------------------------");
    Serial.printf("Temperature:  %.1f °C\n", airTemp);
    Serial.printf("Humidity:     %.1f%%\n", airHumidity);
    Serial.printf("Soil Moisture: %d%%\n", soilMoisturePercent);
    Serial.printf("Water Level:  %s\n", waterState.c_str());
    Serial.println("----------------------------------\n");

    // Display data on OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_haxrcorp4089_tr);

    u8g2.drawStr(2, 13, "Smart Plant Pot");
    u8g2.drawLine(0, 18, 128, 18);

    u8g2.drawStr(3, 29, "Air temp:");
    u8g2.setCursor(100, 29);
    u8g2.print(roundedTemp);
    u8g2.print(" C");

    u8g2.drawStr(3, 39, "Air humid:");
    u8g2.setCursor(100, 39);
    u8g2.print((int)airHumidity);
    u8g2.print("%");

    u8g2.drawStr(3, 49, "Soil moist:");
    u8g2.setCursor(100, 49);
    u8g2.print(soilMoisturePercent);
    u8g2.print("%");

    u8g2.drawStr(3, 59, "Water lvl:");
    u8g2.setCursor(100, 59);
    u8g2.print(waterState);

    u8g2.sendBuffer();

    Serial.println("OLED updated.");
    delay(5000);
}
