#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>

// ESP32 pin definitions
#define DHTPIN 2           
#define DHTTYPE DHT11       
#define MOISTURE_PIN 35     
#define WATERLEVEL_PIN 33  

// Calibration values
int dryValue = 3300;    // Raw reading when soil is dry
int wetValue = 1300;    // Raw reading when soil is wet
int emptyValue = 0;     // Raw reading when water sensor is empty
int fullValue = 4095;   // Raw reading when water sensor is full

// Initialize OLED display using hardware I2C (SCL=22, SDA=21)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, 22, 21);  // Inverted mode

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Icon bitmaps
static const unsigned char image_plant_bits[] = {
  0x00,0xc0,0x00,0xf0,0x04,0xf8,0x0e,0xfc,0x1f,0xfc,0x3f,0x7e,0x3f,0x7e,0x3f,0x3e,
  0x1e,0x0f,0x0c,0x03,0x08,0x01,0x10,0x01,0xa0,0x00,0xc0,0x00,0x80,0x00,0x80,0x00
};

float airTemp = 0;
float airHumidity = 0;
int soilMoisture = 0;
int waterLevel = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  u8g2.begin();
}

void loop() {
  // Read sensor values
  airTemp = dht.readTemperature();
  airHumidity = dht.readHumidity();
  soilMoisture = analogRead(MOISTURE_PIN);
  waterLevel = analogRead(WATERLEVEL_PIN);

  int roundedTemp = round(airTemp);
  int soilMoisturePercent = map(soilMoisture, dryValue, wetValue, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  int waterThreshold = (emptyValue + fullValue) / 2;
  String waterState = (waterLevel > waterThreshold) ? "Full" : "Empty";

  // Clear the display
  u8g2.clearBuffer();
  
  // Fill screen with black (inverted background)
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, 128, 64); // Celá obrazovka černá
  u8g2.setDrawColor(2); // Bílý text na černém pozadí

  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  // Draw plant icon
  u8g2.drawXBM(110, 1, 16, 16, image_plant_bits);

  // Set font and draw title
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(2, 12, "Smart plant pot");
  u8g2.drawLine(0, 18, 127, 18);

  // Display air humidity
  u8g2.drawStr(3, 39, "Air humid:");
  u8g2.setCursor(100, 39);
  u8g2.print((int)airHumidity);
  u8g2.print("%");

  // Display air temperature
  u8g2.drawUTF8(3, 29, "Air temp:");
  u8g2.setCursor(100, 29);
  u8g2.print(roundedTemp);
  u8g2.print(" °C");

  // Display soil moisture
  u8g2.drawStr(3, 49, "Soil moist:");
  u8g2.setCursor(100, 49);
  u8g2.print(soilMoisturePercent);
  u8g2.print("%");

  // Display water level
  u8g2.drawStr(3, 59, "Water lvl:");
  u8g2.setCursor(100, 59);
  u8g2.print(waterState);

  // Send the data to the display
  u8g2.sendBuffer();

  delay(2000);
}
