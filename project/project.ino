#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"

const char* ssid = "Abo Hasan";
const char* password = "12345678";

TFT_eSPI tft = TFT_eSPI();

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
}

void fetchAndShowTemperature() {
  showMessage("Uppdaterar...");
  HTTPClient http;
  http.begin("http://opendata-download-metanalys.smhi.se/api/category/mesan1g/version/2/geotype/point/lon/15.5869/lat/56.1612/data.json");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  Serial.print("HTTP-kod: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Data mottagen!");

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonObject root = doc.as<JsonObject>();
      JsonArray parameters = root["timeSeries"][0]["parameters"];
      for (JsonObject param : parameters) {
        if (param["name"] == "t") { // t = Temperatur
          float temp = param["values"][0];
          Serial.print("Temperatur hittad: ");
          Serial.println(temp);

          tft.fillScreen(TFT_BLACK);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextSize(2);
          tft.drawString("Plats: Karlskrona", 10, 30);
          tft.drawString("Temp: " + String(temp, 1) + " \xB0C", 10, 70);
          return;
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
  // Tom loop
}
