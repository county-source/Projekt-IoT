#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>  // <- nutné pro configTzTime
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>

// ========================
// NASTAVENÍ
// ========================
int SLEEP_START = 23;
int SLEEP_END   = 6;

const char* WIFI_SSID     = "TP-LINK_F964_5G";
const char* WIFI_PASSWORD = "1974197419";
const char* ntpServer     = "pool.ntp.org";

unsigned long lastWifiAttempt = 0;
const unsigned long wifiRetryInterval = 3600000UL;

#define DHTPIN 2
#define DHTTYPE DHT11
#define MOISTURE_PIN 34
#define WATERLEVEL_PIN 33

int dryValue   = 3000;
int wetValue   = 950;
int emptyValue = 0;
int fullValue  = 4095;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);
DHT dht(DHTPIN, DHTTYPE);

float airTemp = 0;
float airHumidity = 0;
int soilMoisture = 0;
int waterLevel = 0;
bool timeAvailable = false;

static const unsigned char image_plant_bits[] = {
  0x00,0xc0,0x00,0xf0,0x04,0xf8,0x0e,0xfc,0x1f,0xfc,0x3f,0x7e,0x3f,0x7e,0x3f,0x3e,
  0x1e,0x0f,0x0c,0x03,0x08,0x01,0x10,0x01,0xa0,0x00,0xc0,0x00,0x80,0x00,0x80,0x00
};

// ========================
// FUNKCE
// ========================

void initTime() {
  // Automatické nastavení CET/CEST časového pásma
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", ntpServer);
  Serial.print("Waiting for NTP sync");
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 10) {
    Serial.print(".");
    delay(1000);
    retry++;
  }
  Serial.println();
  if (getLocalTime(&timeinfo)) {
    Serial.println("Cas byl synchronizovan");
    timeAvailable = true;
  } else {
    Serial.println("Nepodarilo se nacist cas.");
    timeAvailable = false;
  }
}

void tryConnectWiFi() {
  Serial.println("WiFi: Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i < 5; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi: Connected!");
    initTime();
  } else {
    Serial.println("WiFi: Failed.");
    timeAvailable = false;
  }
}

uint64_t calcTimeToHour(int targetHour) {
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);

  int nowSec = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
  int targetSec = targetHour * 3600;
  if (targetHour <= timeinfo.tm_hour) targetSec += 24 * 3600;

  int diff = targetSec - nowSec;
  if (diff < 0) diff = 0;
  return (uint64_t)diff * 1000000ULL;
}

void readAndDisplaySensors() {
  float newAirTemp = dht.readTemperature();
  float newAirHumidity = dht.readHumidity();
  if (!isnan(newAirTemp)) airTemp = newAirTemp;
  if (!isnan(newAirHumidity)) airHumidity = newAirHumidity;

  soilMoisture = analogRead(MOISTURE_PIN);
  waterLevel = analogRead(WATERLEVEL_PIN);

  int roundedTemp = round(airTemp);
  int soilPerc = map(soilMoisture, dryValue, wetValue, 0, 100);
  soilPerc = constrain(soilPerc, 0, 100);
  int waterThreshold = (emptyValue + fullValue) / 2;
  String waterState = (waterLevel > waterThreshold) ? "Full" : "Empty";

  Serial.printf("Temp: %.2f°C, Humidity: %.2f%%, Soil: %d -> %d%%, Water: %d -> %s\n",
    airTemp, airHumidity, soilMoisture, soilPerc, waterLevel, waterState.c_str());

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.drawXBM(111, 1, 16, 16, image_plant_bits);
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(2, 12, "Smart plant pot");
  u8g2.drawLine(0, 18, 127, 18);
  u8g2.drawStr(3, 29, "Air temp:");
  u8g2.setCursor(100, 29);
  u8g2.print(roundedTemp);
  u8g2.print(" °C");
  u8g2.drawStr(3, 39, "Air humid:");
  u8g2.setCursor(100, 39);
  u8g2.print((int)airHumidity);
  u8g2.print("%");
  u8g2.drawStr(3, 49, "Soil moist:");
  u8g2.setCursor(100, 49);
  u8g2.print(soilPerc);
  u8g2.print("%");
  u8g2.drawStr(3, 59, "Water lvl:");
  u8g2.setCursor(100, 59);
  u8g2.print(waterState);
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  u8g2.begin();
  u8g2.setPowerSave(0);
  tryConnectWiFi();
}

void loop() {
  if ((millis() - lastWifiAttempt) >= wifiRetryInterval) {
    lastWifiAttempt = millis();
    tryConnectWiFi();
  }

  if (timeAvailable) {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);

    Serial.printf("Cas: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    if (timeinfo.tm_hour < SLEEP_END || timeinfo.tm_hour >= SLEEP_START) {
      Serial.println("Noc – usinam...");
      uint64_t sleepTime = calcTimeToHour(SLEEP_END);
      u8g2.clearBuffer(); u8g2.sendBuffer(); u8g2.setPowerSave(1);
      delay(60000);
      sleepTime -= 60000000ULL;
      esp_deep_sleep(sleepTime);
    }
  } else {
    Serial.println("Cas neni dostupny – pokracuji bez spanku");
  }

  readAndDisplaySensors();
  delay(5000);
}
