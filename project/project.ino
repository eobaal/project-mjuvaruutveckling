#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "pin_config.h"
#include <algorithm> // For min_element and max_element

void drawTemperatureGraph();
void showCurrentTemperature();
void showWiFiStatus();
void fetchWeatherData();
void showMenu();
void drawWeatherIcon(int x, int y, int code);

const char* ssid = "Abo Hasan";
const char* password = "12345678";

TFT_eSPI tft = TFT_eSPI();

String menuItems[] = {"Weather", "Temperature", "WiFi Status"};
int menuIndex = 0;
bool menuSelected = false;
bool cityChangeRequested = false;

struct City {
  const char* name;
  float lat;
  float lon;
};

City cities[] = {
  {"Karlskrona", 56.1612, 15.5869},
  {"Stockholm", 59.3293, 18.0686},
  {"Goteborg", 57.7089, 11.9746},
  {"Malmo", 55.604981, 13.003822}
};

int cityIndex = 0;

DynamicJsonDocument doc(30000);
std::vector<float> temperatures24h;
float currentTemperature = NAN;
int currentWeatherCode = -1;

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
  City city = cities[cityIndex];
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(city.lat) + "&longitude=" + String(city.lon) + "&hourly=temperature_2m&current_weather=true";

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray temps = doc["hourly"]["temperature_2m"];
      temperatures24h.clear();
      for (int i = 0; i < 24; i++) {
        temperatures24h.push_back(temps[i]);
      }
      currentTemperature = doc["current_weather"]["temperature"].as<float>();
      currentWeatherCode = doc["current_weather"]["weathercode"].as<int>();
    }
  }
  http.end();
}

void showMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Menu:", 10, 10);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(menuItems[menuIndex], 10, 50);
  tft.drawString("City: " + String(cities[cityIndex].name), 10, 80);
  tft.drawString("Press to select", 10, 120);
}

void loop() {
  if (!menuSelected) {
    if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == LOW) {
      delay(500);
      cityIndex++;
      if (cityIndex >= 4) cityIndex = 0;
      fetchWeatherData();
      showMenu();
    }
    else if (digitalRead(PIN_BUTTON_1) == LOW) {
      delay(200);
      menuIndex++;
      if (menuIndex > 2) menuIndex = 0;
      showMenu();
    } else if (digitalRead(PIN_BUTTON_2) == LOW) {
      delay(200);
      menuSelected = true;
    }
  }

  if (millis() - lastUpdate >= updateInterval) {
    fetchWeatherData();
    lastUpdate = millis();
  }

  if (menuSelected) {
    if (menuIndex == 0) {
      drawTemperatureGraph();
    } else if (menuIndex == 1) {
      showCurrentTemperature();
    } else if (menuIndex == 2) {
      showWiFiStatus();
    }
    while (true) {
      if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == LOW) {
        delay(500);
        menuSelected = false;
        showMenu();
        break;
      }
    }
  }
}

void drawTemperatureGraph() {
  tft.fillScreen(TFT_BLACK);
  int x0 = 30, y0 = 140, width = 260, height = 100;
  tft.drawLine(x0, y0, x0 + width, y0, TFT_WHITE);
  tft.drawLine(x0, y0, x0, y0 - height, TFT_WHITE);

  if (temperatures24h.empty()) {
    tft.drawString("No data", 50, 100);
    return;
  }

  float minTemp = *min_element(temperatures24h.begin(), temperatures24h.end());
  float maxTemp = *max_element(temperatures24h.begin(), temperatures24h.end());

  int stepX = width / 23;
  for (int i = 0; i < 23; i++) {
    int x1 = x0 + i * stepX;
    int x2 = x0 + (i + 1) * stepX;
    int y1 = y0 - ((temperatures24h[i] - minTemp) / (maxTemp - minTemp)) * height;
    int y2 = y0 - ((temperatures24h[i+1] - minTemp) / (maxTemp - minTemp)) * height;
    tft.drawLine(x1, y1, x2, y2, TFT_GREEN);
    tft.fillCircle(x1, y1, 2, TFT_RED);
  }
  tft.fillCircle(x0 + 23 * stepX, y0 - ((temperatures24h[23] - minTemp) / (maxTemp - minTemp)) * height, 2, TFT_RED);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  for (int i = 0; i <= 5; i++) {
    float value = minTemp + i * (maxTemp - minTemp) / 5;
    int yPos = y0 - (i * height / 5);
    tft.drawString(String(value, 1) + "C", 0, yPos - 4);
  }

  tft.drawString("0h", x0, y0 + 10);
  tft.drawString("6h", x0 + stepX * 6, y0 + 10);
  tft.drawString("12h", x0 + stepX * 12, y0 + 10);
  tft.drawString("18h", x0 + stepX * 18, y0 + 10);
  tft.drawString("23h", x0 + stepX * 23 - 10, y0 + 10);
}

void drawWeatherIcon(int x, int y, int code) {
  if (code == 0) {
    tft.fillCircle(x, y, 10, TFT_YELLOW);
  } else if (code >= 1 && code <= 3) {
    tft.fillCircle(x, y, 10, TFT_LIGHTGREY);
    tft.fillCircle(x + 10, y, 10, TFT_LIGHTGREY);
    tft.fillCircle(x + 5, y - 5, 10, TFT_LIGHTGREY);
  } else if (code >= 51 && code <= 67) {
    tft.fillCircle(x-5, y, 3, TFT_BLUE);
    tft.fillCircle(x+5, y, 3, TFT_BLUE);
    tft.fillCircle(x, y+10, 3, TFT_BLUE);
  } else if (code >= 95) {
    tft.drawLine(x, y, x+5, y+10, TFT_YELLOW);
    tft.drawLine(x+5, y+10, x, y+20, TFT_YELLOW);
  } else {
    tft.drawString("?", x, y, 2);
  }
}

void showCurrentTemperature() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Location: " + String(cities[cityIndex].name), 10, 30);
  tft.drawString("Temp: " + String(currentTemperature, 1) + " C", 10, 70);
  drawWeatherIcon(250, 50, currentWeatherCode);
}

void showWiFiStatus() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString("WiFi: OK", 10, 50);
  } else {
    tft.drawString("WiFi: LOST", 10, 50);
  }
}
