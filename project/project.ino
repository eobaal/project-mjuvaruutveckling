#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"

// WiFi-inställningar
const char* ssid = "Abo Hasan";
const char* password = "12345678";

// Skärm
TFT_eSPI tft = TFT_eSPI();

// Timer för auto-uppdatering
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10 * 60 * 1000; // 10 minuter

void showMessage(const String& message, uint32_t color = TFT_WHITE) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(message, 10, 50);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  showMessage("Startar...");

  WiFi.begin(ssid, password);
  Serial.println("Ansluter till WiFi...");

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi anslutning misslyckades!");
    showMessage("WiFi fel!", TFT_RED);
    while (true);
  }

  Serial.println("\nWiFi ansluten!");
  showMessage("WiFi ansluten!", TFT_GREEN);
  delay(1000);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Hello students", 10, 10);
  delay(2000);
  tft.drawString("Group 9", 10, 40);
  delay(3000);

  fetchAndShowGraph();
  lastUpdate = millis();
}

void fetchAndShowGraph() {
  showMessage("Uppdaterar...");
  HTTPClient http;

  http.begin("https://api.open-meteo.com/v1/forecast?latitude=56.1612&longitude=15.5869&hourly=temperature_2m&forecast_days=1");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  Serial.print("HTTP-kod: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Data mottagen!");

    DynamicJsonDocument doc(30000); // Större dokumentstorlek

    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray temps = doc["hourly"]["temperature_2m"];

      if (!temps.isNull()) {
        drawTemperatureGraph(temps);
      } else {
        showMessage("Ingen data!", TFT_RED);
      }
    } else {
      Serial.print("JSON Fel: ");
      Serial.println(error.c_str());
      showMessage("JSON Fel!", TFT_RED);
    }
  } else {
    Serial.println("HTTP FEL!");
    showMessage("HTTP Fel!", TFT_RED);
  }

  http.end();
}

void drawTemperatureGraph(JsonArray temps) {
  tft.fillScreen(TFT_BLACK);

  int x0 = 30; // Start x
  int y0 = 140; // Baslinje y
  int width = 260;
  int height = 100;

  tft.drawLine(x0, y0, x0 + width, y0, TFT_WHITE); // X-axel
  tft.drawLine(x0, y0, x0, y0 - height, TFT_WHITE); // Y-axel

  // Hitta min och max temperatur
  float minTemp = temps[0].as<float>();
  float maxTemp = temps[0].as<float>();
  for (int i = 0; i < 24; i++) {
    float value = temps[i].as<float>();
    if (value < minTemp) minTemp = value;
    if (value > maxTemp) maxTemp = value;
  }

  int stepX = width / 23; // 23 steg för 24 punkter

  // Rita graf
  for (int i = 0; i < 23; i++) {
    int x1 = x0 + i * stepX;
    int x2 = x0 + (i + 1) * stepX;

    int y1 = y0 - ((temps[i].as<float>() - minTemp) / (maxTemp - minTemp)) * height;
    int y2 = y0 - ((temps[i + 1].as<float>() - minTemp) / (maxTemp - minTemp)) * height;

    tft.drawLine(x1, y1, x2, y2, TFT_GREEN);
    tft.fillCircle(x1, y1, 2, TFT_RED);
  }

  // Sista punkten
  tft.fillCircle(x0 + 23 * stepX, y0 - ((temps[23].as<float>() - minTemp) / (maxTemp - minTemp)) * height, 2, TFT_RED);

  // Skriv temperatur på Y-axeln
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString(String(maxTemp, 1) + " C", 2, y0 - height);
  tft.drawString(String(minTemp, 1) + " C", 2, y0 - 10);

  // Skriv tid på X-axeln
  tft.drawString("0h", x0 - 5, y0 + 5);
  tft.drawString("6h", x0 + stepX * 6 - 5, y0 + 5);
  tft.drawString("12h", x0 + stepX * 12 - 5, y0 + 5);
  tft.drawString("18h", x0 + stepX * 18 - 5, y0 + 5);
  tft.drawString("23h", x0 + stepX * 23 - 10, y0 + 5);
}

void loop() {
  if (millis() - lastUpdate >= updateInterval) {
    fetchAndShowGraph();
    lastUpdate = millis();
  }
}
