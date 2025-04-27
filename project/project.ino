#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"

const char* ssid = "Abo Hasan";
const char* password = "12345678";

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

  showMessage("Hej students");
  delay(2000);
  showMessage("Group 9");
  delay(3000);

  fetchAndShowTemperature();
  lastUpdate = millis();
}

void fetchAndShowTemperature() {
  showMessage("Uppdaterar...");
  HTTPClient http;
  
  // Open-Meteo API för Karlskrona
  http.begin("https://api.open-meteo.com/v1/forecast?latitude=56.1612&longitude=15.5869&current_weather=true");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  Serial.print("HTTP-kod: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Data mottagen!");

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float temp = doc["current_weather"]["temperature"];
      Serial.print("Temperatur: ");
      Serial.println(temp);

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);
      tft.drawString("Plats: Karlskrona", 10, 30);
      tft.drawString("Temp: " + String(temp, 1) + " \xB0C", 10, 70);
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

void loop() {
  if (millis() - lastUpdate >= updateInterval) {
    fetchAndShowTemperature();
    lastUpdate = millis();
  }
}
