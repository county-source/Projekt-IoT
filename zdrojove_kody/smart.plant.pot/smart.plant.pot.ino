#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp_sleep.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>
#include <time.h>

// Your WiFi Credentials
const char* ssid     = "TP-LINK_F964_5G";
const char* password = "1974197419";

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);  // UTC+1 for Prague

// Sleep Settings
const int sleepStartHour = 22;  // Start deep sleep (e.g., 0 = midnight)
const int sleepEndHour = 6;    // Wake up (e.g., 6 = 6 AM)

// Global Variable to Store Time
int fixedHour = -1;

// ESP32 Pin Definitions
#define DHTPIN 2           
#define DHTTYPE DHT11       
#define MOISTURE_PIN 35     
#define WATERLEVEL_PIN 33  

// Calibration values
int dryValue = 3300;
int wetValue = 1300;
int emptyValue = 0;
int fullValue = 4095;

// Initialize OLED Display using I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, 22, 21);  

// Initialize DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Function to put ESP32 into deep sleep
void goToDeepSleep(int sleepDurationSeconds) {
    Serial.println("\n----------------------------------");
    Serial.printf("💤 ENTERING DEEP SLEEP for %d seconds...\n", sleepDurationSeconds);
    Serial.println("----------------------------------\n");
    delay(200);
    esp_sleep_enable_timer_wakeup(sleepDurationSeconds * 1000000ULL);
    esp_deep_sleep_start();
}

// Function to get current hour correctly
int getCurrentHour() {
    timeClient.update();
    time_t now = timeClient.getEpochTime();
    struct tm rtcTime;
    localtime_r(&now, &rtcTime);

    fixedHour = rtcTime.tm_hour;

    Serial.printf("\n🕐 Current Time: %02d:%02d:%02d\n", rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec);
    return fixedHour;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n🚀 Booting...\n");

    // **Initialize OLED Display**
    u8g2.begin();
    u8g2.setPowerSave(0);

    // **Start DHT Sensor**
    dht.begin();

    // **Connect to WiFi**
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\n✅ WiFi Connected!");

    timeClient.begin();
    timeClient.update();

    getCurrentHour();
}

// **🔥 Read DHT11 with Retry Mechanism**
float readDHTTemperature() {
    float temp;
    for (int i = 0; i < 5; i++) {
        temp = dht.readTemperature();
        if (!isnan(temp)) return temp;
        if (i == 0) Serial.println("⚠️ DHT11 TEMP READ FAILED. RETRYING...");
        delay(1000);
    }
    return -99;
}

float readDHTHumidity() {
    float humidity;
    for (int i = 0; i < 5; i++) {
        humidity = dht.readHumidity();
        if (!isnan(humidity)) return humidity;
        if (i == 0) Serial.println("⚠️ DHT11 HUMIDITY READ FAILED. RETRYING...");
        delay(1000);
    }
    return -99;
}

void loop() {
    int currentHour = getCurrentHour();

    // **🔥 Ensure deep sleep triggers correctly**
    if (currentHour >= sleepStartHour && currentHour < sleepEndHour) {
        goToDeepSleep((sleepEndHour - currentHour) * 3600);
    }

    // **🔥 Read DHT11 with retry**
    float airTemp = readDHTTemperature();
    float airHumidity = readDHTHumidity();

    if (airTemp == -99) airTemp = 0;
    if (airHumidity == -99) airHumidity = 0;

    int soilMoisture = analogRead(MOISTURE_PIN);
    int waterLevel = analogRead(WATERLEVEL_PIN);

    int roundedTemp = round(airTemp);
    int soilMoisturePercent = map(soilMoisture, dryValue, wetValue, 0, 100);
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    int waterThreshold = (emptyValue + fullValue) / 2;
    String waterState = (waterLevel > waterThreshold) ? "Full" : "Empty";

    // **🔥 Print sensor data in a clean format**
    Serial.println("\n----------------------------------");
    Serial.printf("🌡 Temperature:  %.1f °C\n", airTemp);
    Serial.printf("💧 Humidity:     %.1f%%\n", airHumidity);
    Serial.printf("🌱 Soil Moist:   %d%%\n", soilMoisturePercent);
    Serial.printf("🚰 Water Level:  %s\n", waterState.c_str());
    Serial.println("----------------------------------\n");

    // **🔥 Ensure OLED displays correctly**
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_haxrcorp4089_tr);

    u8g2.drawStr(2, 12, "Smart Plant Pot");
    u8g2.drawLine(0, 18, 128, 18);

    u8g2.drawStr(3, 29, "Air Temp:");
    u8g2.setCursor(100, 29);
    u8g2.print(roundedTemp);
    u8g2.print(" C");

    u8g2.drawStr(3, 39, "Air Humid:");
    u8g2.setCursor(100, 39);
    u8g2.print((int)airHumidity);
    u8g2.print("%");

    u8g2.drawStr(3, 49, "Soil Moist:");
    u8g2.setCursor(100, 49);
    u8g2.print(soilMoisturePercent);
    u8g2.print("%");

    u8g2.drawStr(3, 59, "Water Level:");
    u8g2.setCursor(100, 59);
    u8g2.print(waterState);

    u8g2.sendBuffer();

    Serial.println("✅ OLED Updated Successfully.");
    delay(5000);
}
