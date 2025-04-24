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
#include <vector>

String ssid = "Abo Hasan";
String password = "12345678";

TFT_eSPI tft = TFT_eSPI();

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170

WiFiClient wifi_client;

void displayWelcomeMessage() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Hello student", 10, 10);
  tft.drawString("Group 9", 10, 40);
}

std::vector<float> fetchSMHITemperatures() {
  std::vector<float> temps;

  HTTPClient http;
  http.begin("https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.5869/lat/56.1612/data.json"); // Karlskrona
  int httpCode = http.GET();

  Serial.print("HTTP response code: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();

    DynamicJsonDocument doc(60000);
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      return temps;
    }

    JsonArray times = doc["timeSeries"];
    int count = 0;
    for (JsonObject entry : times) {
      if (count >= 24) break;

      for (JsonObject param : entry["parameters"].as<JsonArray>()) {
        if (param["name"] == "t") {
          float temp = param["values"][0];
          temps.push_back(temp);
          Serial.print("Timme ");
          Serial.print(count);
          Serial.print(": ");
          Serial.print(temp);
          Serial.println(" C");
          count++;
          break;
        }
      }
    }
  } else {
    Serial.println("Kunde inte hämta väderdata från SMHI.");
  }

  http.end();
  return temps;
}

void drawTemperatureGraph(const std::vector<float>& temps) {
  if (temps.empty()) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.drawString("Ingen data", 50, 80);
    return;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  int x0 = 30;
  int y0 = 140;
  int width = 260;
  int height = 100;

  tft.drawLine(x0, y0, x0 + width, y0, TFT_WHITE);     // X-axel (tid)
  tft.drawLine(x0, y0, x0, y0 - height, TFT_WHITE);    // Y-axel (temp)

  float minTemp = temps[0];
  float maxTemp = temps[0];
  for (float t : temps) {
    if (t < minTemp) minTemp = t;
    if (t > maxTemp) maxTemp = t;
  }

  int n = temps.size();
  int stepX = width / (n - 1);

  for (int i = 0; i < n - 1; ++i) {
    int x1 = x0 + i * stepX;
    int x2 = x0 + (i + 1) * stepX;

    int y1 = y0 - ((temps[i] - minTemp) / (maxTemp - minTemp)) * height;
    int y2 = y0 - ((temps[i + 1] - minTemp) / (maxTemp - minTemp)) * height;

    tft.drawLine(x1, y1, x2, y2, TFT_GREEN);
    tft.fillCircle(x1, y1, 2, TFT_RED);
  }

  tft.fillCircle(x0 + (n - 1) * stepX, y0 - ((temps[n - 1] - minTemp) / (maxTemp - minTemp)) * height, 2, TFT_RED);

  tft.drawString(String(maxTemp, 1) + " C", 2, y0 - height);
  tft.drawString(String(minTemp, 1) + " C", 2, y0 - 10);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Starting ESP32 program...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Connecting to WiFi...", 10, 10);
    Serial.println("Attempting to connect to WiFi...");
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Connected to WiFi", 10, 10);
  Serial.println("Connected to WiFi");

  displayWelcomeMessage();
  delay(5000);
  tft.fillScreen(TFT_BLACK);

  std::vector<float> temps = fetchSMHITemperatures();
  drawTemperatureGraph(temps);
}

void loop() {
  // Körs inte – all logik finns i setup() för tillfället
}

// TFT Pin check
//////////////////
// DO NOT TOUCH //
//////////////////
#if PIN_LCD_WR  != TFT_WR || \
    PIN_LCD_RD  != TFT_RD || \
    PIN_LCD_CS    != TFT_CS   || \
    PIN_LCD_DC    != TFT_DC   || \
    PIN_LCD_RES   != TFT_RST  || \
    PIN_LCD_D0   != TFT_D0  || \
    PIN_LCD_D1   != TFT_D1  || \
    PIN_LCD_D2   != TFT_D2  || \
    PIN_LCD_D3   != TFT_D3  || \
    PIN_LCD_D4   != TFT_D4  || \
    PIN_LCD_D5   != TFT_D5  || \
    PIN_LCD_D6   != TFT_D6  || \
    PIN_LCD_D7   != TFT_D7  || \
    PIN_LCD_BL   != TFT_BL  || \
    TFT_BACKLIGHT_ON   != HIGH  || \
    170   != TFT_WIDTH  || \
    320   != TFT_HEIGHT
#error  "Error! Please make sure <User_Setups/Setup206_LilyGo_T_Display_S3.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#error  "The current version is not supported for the time being, please use a version below Arduino ESP32 3.0"
#endif
