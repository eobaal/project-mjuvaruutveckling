#include <Arduino.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"
#include <SPI.h>
#include "pin_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>

String ssid = "Abo Hasan";
String password = "12345678";

TFT_eSPI tft = TFT_eSPI();
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170
WiFiClient wifi_client;

void fetchWeatherData(float temps[], String times[], int maxCount) {
  HTTPClient http;
  String url = "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/18.0686/lat/59.3293/data.json";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(120000);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      JsonArray series = doc["timeSeries"];
      int count = 0;
      for (JsonObject entry : series) {
        if (count >= maxCount) break;
        String time = entry["validTime"].as<String>();
        JsonArray parameters = entry["parameters"];
        for (JsonObject param : parameters) {
          if (param["name"] == "t") {
            float value = param["values"][0].as<float>();
            if (value > -50 && value < 60) {
              temps[count] = value;
              times[count] = time.substring(11, 16);
              count++;
            }
            break;
          }
        }
      }
    }
  }
  http.end();
}

void drawWeatherGraph(float temps[], String times[], int count) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("Temp kommande 24h", 10, 5);
  if (count == 0) {
    tft.drawString("Ingen data", 10, 20);
    return;
  }

  float minTemp = temps[0], maxTemp = temps[0];
  for (int i = 1; i < count; i++) {
    if (temps[i] < minTemp) minTemp = temps[i];
    if (temps[i] > maxTemp) maxTemp = temps[i];
  }
  float padding = 2.0;
  minTemp -= padding;
  maxTemp += padding;
  float scale = (DISPLAY_HEIGHT - 60) / (maxTemp - minTemp);
  int graphWidth = DISPLAY_WIDTH - 60;
  int stepX = graphWidth / (count - 1);
  int baseY = DISPLAY_HEIGHT - 30;

  // Rita axlar
  tft.drawLine(30, 20, 30, baseY, TFT_WHITE);
  tft.drawLine(30, baseY, DISPLAY_WIDTH - 20, baseY, TFT_WHITE);

  // Rita temperaturkurva
  for (int i = 0; i < count - 1; i++) {
    int x1 = 30 + i * stepX;
    int y1 = baseY - (temps[i] - minTemp) * scale;
    int x2 = 30 + (i + 1) * stepX;
    int y2 = baseY - (temps[i + 1] - minTemp) * scale;
    tft.drawLine(x1, y1, x2, y2, TFT_CYAN);
  }

  // Y-axel etiketter
  for (int i = 0; i <= 5; i++) {
    float t = minTemp + i * (maxTemp - minTemp) / 5;
    int y = baseY - (t - minTemp) * scale;
    tft.drawString(String(t, 1) + " C", 0, y - 3);
  }

  // X-axel etiketter
  for (int i = 0; i < count; i += 3) {
    int x = 30 + i * stepX;
    if (x < DISPLAY_WIDTH - 20) {
      tft.drawString(times[i], x - 10, baseY + 5);
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Connecting to WiFi...", 10, 10);
    Serial.println("Connecting to WiFi...");
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Connected to WiFi", 10, 10);
  Serial.println("Connectied to WiFi.");
}

void loop() {
  float temps[24];
  String times[24];
  fetchWeatherData(temps, times, 24);
  drawWeatherGraph(temps, times, 24);
  delay(60000);
}
