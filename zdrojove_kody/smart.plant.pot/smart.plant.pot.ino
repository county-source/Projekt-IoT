#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>

//ESP32
#define DHTPIN 2           
#define DHTTYPE DHT11       
#define MOISTURE_PIN 35     
#define WATERLEVEL_PIN 39   

//Arduino
/*
#define DHTPIN 2            
#define DHTTYPE DHT11       
#define MOISTURE_PIN A0     
#define WATERLEVEL_PIN A1
*/

// Initialize OLED display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Icon bitmaps
static const unsigned char image_plant_bits[] = {
  0x00,0xc0,0x00,0xf0,0x04,0xf8,0x0e,0xfc,0x1f,0xfc,0x3f,0x7e,0x3f,0x7e,0x3f,0x3e,
  0x1e,0x0f,0x0c,0x03,0x08,0x01,0x10,0x01,0xa0,0x00,0xc0,0x00,0x80,0x00,0x80,0x00
};

static const unsigned char image_weather_humidity_bits[] = {
  0x20,0x00,0x20,0x00,0x30,0x00,0x70,0x00,0x78,0x00,0xf8,0x00,0xfc,0x01,0xfc,0x01,
  0x7e,0x03,0xfe,0x02,0xff,0x06,0xff,0x07,0xfe,0x03,0xfe,0x03,0xfc,0x01,0xf0,0x00
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

  int soilMoisturePercent = map(soilMoisture, 4095, 0, 0, 100);
  int waterLevelPercent = map(waterLevel, 0, 4095, 0, 100);

  int roundedTemp = round(airTemp);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  // plant icon
  u8g2.drawXBM(111, 1, 16, 16, image_plant_bits);

  //font
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(2, 12, "Smart plant pot");
  u8g2.drawLine(0, 18, 127, 18);


  //air moisture
  u8g2.drawStr(3, 39, "Air moisture:");
  u8g2.setCursor(100, 39);
  u8g2.print((int)airHumidity);
  u8g2.print(" %");

  //air temperature
  u8g2.drawUTF8(3, 29, "Air temp:");
  u8g2.setCursor(100, 29);
  u8g2.print(roundedTemp);
  u8g2.print(" °C");

  //soil moisture percentage
  u8g2.drawStr(3, 49, "Soil moisture:");
  u8g2.setCursor(100, 49);
  u8g2.print(soilMoisturePercent);
  u8g2.print(" %");

  //water level percentage
  u8g2.drawStr(3, 59, "Water level:");
  u8g2.setCursor(100, 59);
  u8g2.print(waterLevelPercent);
  u8g2.print(" %");

  //data to the display
  u8g2.sendBuffer();

  //serial for debugging
  Serial.print("Temp: ");
  Serial.print(airTemp);
  Serial.print(" C, Humidity: ");
  Serial.print(airHumidity);
  Serial.print(" %, Soil: ");
  Serial.print(soilMoisture);
  Serial.print(" (");
  Serial.print(soilMoisturePercent);
  Serial.print("%), Water: ");
  Serial.print(waterLevel);
  Serial.print(" (");
  Serial.print(waterLevelPercent);
  Serial.println("%)");

  delay(2000);
}
