#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"

// Funktionsdeklarationer
void drawTemperatureGraph(ArduinoJson::JsonArray temps);
void showCurrentTemperature();
void showWiFiStatus();
void fetchWeatherData();
void showMenu();

const char* ssid = "Abo Hasan";
const char* password = "12345678";

TFT_eSPI tft = TFT_eSPI();

String menuItems[] = {"Vädergraf", "Temperatur", "WiFi Status"};
int menuIndex = 0;
bool menuSelected = false;

ArduinoJson::JsonArray currentTemps;
DynamicJsonDocument doc(30000);
float currentTemperature = NAN;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10 * 60 * 1000;

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Hello students", 10, 10);
  delay(2000);
  tft.drawString("Group 9", 10, 40);
  delay(3000);

  WiFi.begin(ssid, password);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Connecting WiFi...", 10, 10);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("WiFi Failed", 10, 50);
    Serial.println("WiFi Failed!");
    while (true);
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WiFi Connected!", 10, 10);
  Serial.println("WiFi Connected!");

  delay(2000);
  fetchWeatherData();
  showMenu();
  lastUpdate = millis();
}

void fetchWeatherData() {
  HTTPClient http;
  http.begin("https://api.open-meteo.com/v1/forecast?latitude=56.1612&longitude=15.5869&hourly=temperature_2m&current_weather=true");

  int httpCode = http.GET();
  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      if (doc.containsKey("hourly") && doc["hourly"].containsKey("temperature_2m")) {
        currentTemps = doc["hourly"]["temperature_2m"];
      }
      if (doc.containsKey("current_weather") && doc["current_weather"].containsKey("temperature")) {
        currentTemperature = doc["current_weather"]["temperature"].as<float>();
      }
    } else {
      Serial.print("JSON Fel: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.println("HTTP Fel!");
  }

  http.end();
}

void showMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Meny:", 10, 10);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(menuItems[menuIndex], 10, 50);
  tft.drawString("Tryck för att välja", 10, 100);
}

void loop() {
  if (digitalRead(PIN_BUTTON_1) == LOW) {
    delay(200);
    menuIndex++;
    if (menuIndex > 2) menuIndex = 0;
    showMenu();
  }

  if (digitalRead(PIN_BUTTON_2) == LOW) {
    delay(200);
    menuSelected = true;
  }

  if (millis() - lastUpdate >= updateInterval) {
    fetchWeatherData();
    lastUpdate = millis();
  }

  if (currentTemps.isNull()) {
    showMenu();
  }

  if (menuSelected) {
    if (menuIndex == 0) {
      drawTemperatureGraph(currentTemps);
    } else if (menuIndex == 1) {
      showCurrentTemperature();
    } else if (menuIndex == 2) {
      showWiFiStatus();
    }
    delay(3000);
    showMenu();
    menuSelected = false;
  }
}

void drawTemperatureGraph(ArduinoJson::JsonArray temps) {
  tft.fillScreen(TFT_BLACK);

  int x0 = 30;
  int y0 = 140;
  int width = 260;
  int height = 100;

  tft.drawLine(x0, y0, x0 + width, y0, TFT_WHITE);
  tft.drawLine(x0, y0, x0, y0 - height, TFT_WHITE);

  float minTemp = temps[0].as<float>();
  float maxTemp = temps[0].as<float>();

  for (int i = 0; i < 24; i++) {
    float temp = temps[i].as<float>();
    if (temp < minTemp) minTemp = temp;
    if (temp > maxTemp) maxTemp = temp;
  }

  int stepX = width / 23;

  for (int i = 0; i < 23; i++) {
    int x1 = x0 + i * stepX;
    int x2 = x0 + (i + 1) * stepX;

    int y1 = y0 - ((temps[i].as<float>() - minTemp) / (maxTemp - minTemp)) * height;
    int y2 = y0 - ((temps[i + 1].as<float>() - minTemp) / (maxTemp - minTemp)) * height;

    tft.drawLine(x1, y1, x2, y2, TFT_GREEN);
    tft.fillCircle(x1, y1, 2, TFT_RED);
  }

  tft.fillCircle(x0 + 23 * stepX, y0 - ((temps[23].as<float>() - minTemp) / (maxTemp - minTemp)) * height, 2, TFT_RED);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  int divisions = 5;
  for (int i = 0; i <= divisions; i++) {
    float value = minTemp + i * (maxTemp - minTemp) / divisions;
    int yPos = y0 - (i * height / divisions);
    tft.drawString(String(value, 1) + "C", 0, yPos - 4);
  }

  tft.drawString("0h", x0, y0 + 10);
  tft.drawString("6h", x0 + stepX * 6, y0 + 10);
  tft.drawString("12h", x0 + stepX * 12, y0 + 10);
  tft.drawString("18h", x0 + stepX * 18, y0 + 10);
  tft.drawString("23h", x0 + stepX * 23 - 10, y0 + 10);
}

void showCurrentTemperature() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Plats: Karlskrona", 10, 30);
  tft.drawString("Temp: " + String(currentTemperature, 1) + " C", 10, 70);
}

void showWiFiStatus() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString("WiFi: OK", 10, 50);
  } else {
    tft.drawString("WiFi: LOST", 10, 50);
  }
}
