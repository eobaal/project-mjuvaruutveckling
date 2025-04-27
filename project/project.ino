#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"

// WiFi-uppgifter
const char* ssid = "Abo Hasan";
const char* password = "12345678";

TFT_eSPI tft = TFT_eSPI();

// Timer
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
  Serial.println("Startar setup...");

  tft.init();
  tft.setRotation(1);
  showMessage("Startar...");

  delay(500);

  WiFi.begin(ssid, password);
  Serial.println("Ansluter till WiFi...");
  showMessage("Ansluter WiFi...");

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi anslutning misslyckades!");
    showMessage("WiFi fel!", TFT_RED);
    while (true); // Stoppa allt
  }

  Serial.println("WiFi ansluten!");
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
  Serial.println("Försöker hämta SMHI-data...");
  showMessage("Uppdaterar...");

  HTTPClient http;
  http.begin("http://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.5869/lat/56.1612/data.json");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  Serial.print("HTTP-kod: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(20000);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray timeSeries = doc["timeSeries"];
      if (!timeSeries.isNull()) {
        JsonObject firstTime = timeSeries[0];
        JsonArray parameters = firstTime["parameters"];
        for (JsonObject param : parameters) {
          if (param["name"] == "t") {
            float temp = param["values"][0];
            Serial.print("TEMPERATUR: ");
            Serial.println(temp);

            tft.fillScreen(TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextSize(2);
            tft.drawString("Plats: Karlskrona", 10, 30);
            tft.drawString("Temp: " + String(temp, 1) + " \xB0C", 10, 70);
            return;
          }
        }
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

void loop() {
  if (millis() - lastUpdate >= updateInterval) {
    fetchAndShowTemperature();
    lastUpdate = millis();
  }
}
