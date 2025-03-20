#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>

// ========================
// NASTAVENÍ
// ========================
int SLEEP_START = 23; // Od kdy spát (noční režim)
int SLEEP_END   = 6;  // Do kdy spát

// Vaše Wi‑Fi přihlašovací údaje
const char* WIFI_SSID     = "TP-LINK_F964_5G";
const char* WIFI_PASSWORD = "1974197419";
const char* ntpServer     = "pool.ntp.org";

// Pro zimní čas (ČR): CET = UTC+1
const long gmtOffset_sec      = 3600;    
const int  daylightOffset_sec = 0;

unsigned long lastWifiAttempt = 0;
const unsigned long wifiRetryInterval = 3600000UL; // 1 hodina v ms

// ========================
// HARDWAROVÉ DEFINICE
// ========================
#define DHTPIN 2
#define DHTTYPE DHT11
#define MOISTURE_PIN 35
#define WATERLEVEL_PIN 33

// Kalibrační hodnoty
int dryValue   = 3300;
int wetValue   = 1300;
int emptyValue = 0;
int fullValue  = 4095;

// Inicializace OLED displeje
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

// Inicializace DHT senzoru
DHT dht(DHTPIN, DHTTYPE);

float airTemp      = 0;
float airHumidity  = 0;
int soilMoisture   = 0;
int waterLevel     = 0;

// Ikony (ponechte původní bitmapy)
static const unsigned char image_plant_bits[] = {
  0x00,0xc0,0x00,0xf0,0x04,0xf8,0x0e,0xfc,0x1f,0xfc,0x3f,0x7e,0x3f,0x7e,0x3f,0x3e,
  0x1e,0x0f,0x0c,0x03,0x08,0x01,0x10,0x01,0xa0,0x00,0xc0,0x00,0x80,0x00,0x80,0x00
};

static const unsigned char image_weather_humidity_bits[] = {
  0x20,0x00,0x20,0x00,0x30,0x00,0x70,0x00,0x78,0x00,0xf8,0x00,0xfc,0x01,0xfc,0x01,
  0x7e,0x03,0xfe,0x02,0xff,0x06,0xff,0x07,0xfe,0x03,0xfe,0x03,0xfc,0x01,0xf0,0x00
};

// ========================
// FUNKCE
// ========================

// Synchronizace času pomocí NTP (s offsetem pro zimní čas)
void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Waiting for NTP sync");
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 10) {
    Serial.print(".");
    delay(1000);
    retry++;
  }
  Serial.println();
  if(getLocalTime(&timeinfo)) {
    Serial.printf("Time synchronized: %02d:%02d:%02d\n", 
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println("Failed to synchronize time.");
  }
}

// Pokus o připojení k Wi‑Fi
void tryConnectWiFi() {
  if(WiFi.status() == WL_CONNECTED) return;
  Serial.println("WiFi: Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  for (int i = 0; i < 20; i++) {
    if(WiFi.status() == WL_CONNECTED) break;
    delay(500);
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi: Connected!");
    initTime();
  } else {
    Serial.println("WiFi: Failed.");
  }
}

// Spočítá čas (v mikrosekundách) do dosažení cílové hodiny (0–23).
// Pokud již cílová hodina nastala, počítá do ní další den.
uint64_t calcTimeToHour(int targetHour) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    // Pokud se nepodaří načíst čas, spíme 2 hodiny
    return 2ULL * 3600ULL * 1000000ULL; 
  }
  int hour = timeinfo.tm_hour;
  int min  = timeinfo.tm_min;
  int sec  = timeinfo.tm_sec;
  
  int nowSec    = hour * 3600 + min * 60 + sec;
  int targetSec = targetHour * 3600;
  
  if(targetHour <= hour) {
    targetSec += 24 * 3600;
  }
  int diff = targetSec - nowSec;
  if(diff < 0) diff = 0;
  return (uint64_t)diff * 1000000ULL;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  u8g2.begin();
  // Zajistíme, že displej je aktivní
  u8g2.setPowerSave(0);
  tryConnectWiFi();
}

void loop() {
  // Občas zkusíme opětovné připojení k Wi‑Fi
  if((millis() - lastWifiAttempt) >= wifiRetryInterval) {
    lastWifiAttempt = millis();
    tryConnectWiFi();
  }
  
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    Serial.printf("Aktuální čas: %02d:%02d:%02d, Datum: %04d-%02d-%02d\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    int hour = timeinfo.tm_hour;
    
    // Noční režim: před 6:00 nebo od 22:00 – deep sleep
    if(hour < SLEEP_END || hour >= SLEEP_START) {
      Serial.printf("Je noc (%d), jdu spat do %d:00...\n", hour, SLEEP_END);
      uint64_t sleepTime = calcTimeToHour(SLEEP_END);
      Serial.printf("Usinam na (s): %lu\n", (unsigned long)(sleepTime / 1000000ULL));
      
      // Před usnutím vymažeme displej a vypneme jej (power save)
      u8g2.clearBuffer();
      u8g2.sendBuffer();
      u8g2.setPowerSave(1);
      delay(60000); // čekáme 1 minutu
      sleepTime -= 60000000ULL; // odečteme minutu z doby spánku
      
      esp_deep_sleep(sleepTime);
    }
    else {
      // Denní režim: mezi 6:00 a 22:00 – zařízení běží normálně
      Serial.printf("Je den (%d). Mereni bezi...\n", hour);
      
      // Čtení senzorů
      airTemp      = dht.readTemperature();
      airHumidity  = dht.readHumidity();
      soilMoisture = analogRead(MOISTURE_PIN);
      waterLevel   = analogRead(WATERLEVEL_PIN);
      
      int roundedTemp = round(airTemp);
      int soilPerc = map(soilMoisture, dryValue, wetValue, 0, 100);
      soilPerc = constrain(soilPerc, 0, 100);
      int waterThreshold = (emptyValue + fullValue) / 2;
      String waterState = (waterLevel > waterThreshold) ? "Full" : "Empty";
      
      // Aktualizace OLED displeje
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
      
      Serial.printf("Temp: %.2f °C, Humidity: %.2f%%, Soil: %d -> %d%%, Water: %d -> %s\n",
                    airTemp, airHumidity, soilMoisture, soilPerc, waterLevel, waterState.c_str());
      
      delay(5000); 
    }
  } else {
    Serial.println("Nepodařilo se nacist cas, cekam 5s...");
    delay(5000);
  }
}
