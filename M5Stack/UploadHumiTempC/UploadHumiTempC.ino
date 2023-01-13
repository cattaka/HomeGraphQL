#include <M5StickC.h>
//#include <M5StickCPlus.h>
#include "DHT12.h"
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include "Config.h"

#define REQUEST_DELAY 600000 // 10 minutes

#define CYAN 0x07FF
#define ORANGE 0xFDA0
#define LIGHTGRAY 0xC618
#define YELLOW 0xFFE0
#define FONT_SIZE 10

// M5StickCが発熱するので、それの補正用
#define OFFSET_DEGREE 5.0

#define POST_QUERY "{\"query\":\"mutation { createTempHumiValue( input: { tempHumiValue: { sensorName: \\\"%s\\\", temperature: \\\"%.2f\\\", humidity: \\\"%.2f\\\", pressure: \\\"%.1f\\\", co2: \\\"%d\\\" } } ) { tempHumiValue { id } } }\"}"

byte MhZ19Request[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
 
struct MhZ19Response {
  uint16_t header;
  byte vh;
  byte vl;
  byte temp;
  byte tails[4];
} __attribute__((packed));;

DHT12 dht12; 
Adafruit_BMP280 bme;

long gLastRequestDelay;
long gLastMillis;

#define UPDATE_INTERVAL 15000

uint16_t gClock = UPDATE_INTERVAL;
uint8_t gLedCount = 15;

float calcPws(float degree) {
  float pc = 22120;
  float tc = 647.3;
  float a = -7.76451;
  float b = 1.45838;
  float c = -2.7758;
  float d = -1.23303;
  float x = 1.0 - (degree + 273.15) / tc;
  return pc * exp((a*x + b*pow(x, 1.5) + c*pow(x, 3) + d*pow(x,6))/(1 - x));
}

void ensureWifi() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(LIGHTGRAY);
  M5.Lcd.drawString("Ensuring WiFi...", 0, 30);
  if (WiFi.waitForConnectResult() == WL_DISCONNECTED) {
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.drawString("Failed! rebooting...", 0, 70);
    delay(500);
    esp_restart();
  }
}

word readMhZ19() {
  struct MhZ19Response response;
 
  Serial1.write(MhZ19Request, sizeof MhZ19Request);
  Serial1.readBytes((char *)&response, 9);
 
  if (response.header != 0x86ff) {
//    Serial.println(response.header, HEX);
//    delay(1000);
    return -1;
  }
 
  word ppm = (response.vh << 8) + response.vl;
  byte temp = response.temp - 40;
  return ppm;
}

void setup() {
  M5.begin();
  WiFi.begin(SSID, PASSWORD);
  ensureWifi();
  Wire.begin(0,26);
  pinMode(M5_BUTTON_HOME, INPUT);

  while (!bme.begin(0x76)){  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    delay(100);
  }

  gLastMillis = millis();
  gLastRequestDelay = 0;

  Serial1.begin(9600, SERIAL_8N1, 33, 32);
}

void loop() {
  if(digitalRead(M5_BUTTON_HOME) == LOW){
    gLedCount++;
    if(gLedCount >= 16)
      gLedCount = 7;
    while(digitalRead(M5_BUTTON_HOME) == LOW);
    M5.Axp.ScreenBreath(gLedCount);
  }

  delay(50);
  gClock += 50;
  if (gClock < UPDATE_INTERVAL) {
    return;
  }
  gClock = 0;

  float origTemperatureC = dht12.readTemperature();
  float origHumidity = dht12.readHumidity();
  float temperatureC = origTemperatureC - OFFSET_DEGREE;
  float humidity = origHumidity * calcPws(origTemperatureC) / calcPws(temperatureC);
  float pressure = bme.readPressure();
  digitalWrite(G33, HIGH);

  word co2Ppm = readMhZ19();

  M5.Lcd.fillScreen(BLACK);
  char buf[44];
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawString(SENSOR_NAME, FONT_SIZE * 0, FONT_SIZE * 1);
  M5.Lcd.setTextColor(ORANGE);
  M5.Lcd.drawString("Temperature", FONT_SIZE * 0, FONT_SIZE * 3);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.drawString("Humidity", FONT_SIZE * 0, FONT_SIZE * 6);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.drawString("Pressure", FONT_SIZE * 0, FONT_SIZE * 9);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.drawString("CO2 ppm", FONT_SIZE * 0, FONT_SIZE * 11);

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  sprintf(buf, "%.2f C", temperatureC);
  M5.Lcd.drawString(buf, FONT_SIZE * 1, FONT_SIZE * 4);
  sprintf(buf, "%.2f %%", humidity);
  M5.Lcd.drawString(buf, FONT_SIZE * 1, FONT_SIZE * 7);
  sprintf(buf, "% 6.1f", pressure);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawString(buf, FONT_SIZE * 1, FONT_SIZE * 10);
  sprintf(buf, "%4d", co2Ppm);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawString(buf, FONT_SIZE * 2, FONT_SIZE * 12);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(LIGHTGRAY);
  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.drawString(WiFi.localIP().toString(), 0, FONT_SIZE * 14);
  } else {
    char* message = "UNKNOWN";
    switch(WiFi.status()) {
    case WL_IDLE_STATUS:
      message = "IDLE_STATUS";
      break;
    case WL_NO_SSID_AVAIL:
      message = "NO_SSID_AVAIL";
      break;
    case WL_SCAN_COMPLETED:
      message = "SCAN_COMPLETED";
      break;
    case WL_CONNECTED:
      message = "CONNECTED";
      break;
    case WL_CONNECT_FAILED:
      message = "CONNECT_FAILED";
      break;
    case WL_CONNECTION_LOST:
      message = "CONNECTION_LOST";
      break;
    case WL_DISCONNECTED:
      message = "DISCONNECTED";
      break;
    }
    M5.Lcd.drawString(message, 0, FONT_SIZE * 14);
  }

  if (gLastRequestDelay > 0) {
    unsigned long t = millis();
    gLastRequestDelay -= t - gLastMillis;
    gLastMillis = t;
  } else if (WiFi.status() == WL_CONNECTED) {
    char postQuery[512];
    sprintf(postQuery, POST_QUERY, SENSOR_NAME, temperatureC, humidity, pressure, co2Ppm);
    Serial.println(postQuery);
    for (int i=0;i<3;i++) {
      HTTPClient http;
      if (http.begin(GRAPHQL_ENDPOINT)) {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Accept", "application/json");
        http.addHeader("Authorization", GRAPHQL_ACCESS_TOKEN);
        
        Serial.println("begin succeed");
        int statusCode = http.POST((uint8_t*)postQuery, strlen(postQuery));
        if (200 <= statusCode && statusCode < 300) {
          gLastRequestDelay = REQUEST_DELAY;
        }
        http.end();
        Serial.println(statusCode, DEC);
        break;
      } else {
        Serial.println("begin failed");
        http.end();
      }
    }
  } else {
    esp_restart();
  }
}
