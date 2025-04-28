#include <WiFi.h>            // Library to connect to WiFi
#include <HTTPClient.h>      // Library to send web requests (HTTP)
#include <ArduinoJson.h>     // Library to work with JSON data (weather data)
#include <TFT_eSPI.h>        // Library to control the screen
#include "pin_config.h"      // Your file with button pin numbers

// === Function Declarations ===
// (Telling the program which functions exist)
void drawTemperatureGraph(ArduinoJson::JsonArray temps);
void showCurrentTemperature();
void showWiFiStatus();
void fetchWeatherData();
void showMenu();

// === Global Variables ===
const char* ssid = "Abo Hasan";              // WiFi network name
const char* password = "12345678";            // WiFi password

TFT_eSPI tft = TFT_eSPI();                    // Create screen object

String menuItems[] = {"Weather Graph", "Temperature", "WiFi Status"}; // Menu options
int menuIndex = 0;                            // Current selected menu
bool menuSelected = false;                    // If menu option is selected

ArduinoJson::JsonArray currentTemps;          // Temperatures for 24 hours
DynamicJsonDocument doc(30000);               // JSON document to hold data (max 30 KB)
float currentTemperature = NAN;               // Current temperature

unsigned long lastUpdate = 0;                 // Last weather update time
const unsigned long updateInterval = 10 * 60 * 1000; // Update every 10 minutes

// === setup() runs once when ESP32 starts ===
void setup() {
  Serial.begin(115200);          // Start serial monitor
  tft.init();                    // Start the screen
  tft.setRotation(1);            // Rotate screen to landscape mode
  tft.fillScreen(TFT_BLACK);     // Make the screen black

  pinMode(PIN_BUTTON_1, INPUT_PULLUP); // Set button 1 as input
  pinMode(PIN_BUTTON_2, INPUT_PULLUP); // Set button 2 as input

  // Show welcome text
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Hello students", 10, 10);
  delay(2000);
  tft.drawString("Group 9", 10, 40);
  delay(3000);

  // Try to connect to WiFi
  WiFi.begin(ssid, password);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Connecting WiFi...", 10, 10);

  // Wait for WiFi to connect, max 15 seconds
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) { // If WiFi failed
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("WiFi Failed", 10, 50);
    Serial.println("WiFi Failed!");
    while (true); // Stop here forever
  }

  // If WiFi connected
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WiFi Connected!", 10, 10);
  Serial.println("WiFi Connected!");

  delay(2000);
  fetchWeatherData(); // Get weather data
  showMenu();         // Show the menu
  lastUpdate = millis(); // Save the current time
}

// === Function to fetch weather data ===
void fetchWeatherData() {
  HTTPClient http;
  http.begin("https://api.open-meteo.com/v1/forecast?latitude=56.1612&longitude=15.5869&hourly=temperature_2m&current_weather=true");

  int httpCode = http.GET(); // Send GET request
  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode == 200) { // If success
    String payload = http.getString();
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Get temperature data if available
      if (doc.containsKey("hourly") && doc["hourly"].containsKey("temperature_2m")) {
        currentTemps = doc["hourly"]["temperature_2m"];
      }
      if (doc.containsKey("current_weather") && doc["current_weather"].containsKey("temperature")) {
        currentTemperature = doc["current_weather"]["temperature"].as<float>();
      }
    } else {
      Serial.print("JSON Error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.println("HTTP Error!");
  }

  http.end(); // Close the connection
}

// === Function to show the menu ===
void showMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Menu:", 10, 10);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(menuItems[menuIndex], 10, 50);
  tft.drawString("Press to select", 10, 100);
}

// === loop() runs all the time ===
void loop() {
  if (digitalRead(PIN_BUTTON_1) == LOW) { // Button 1 pressed
    delay(200);
    menuIndex++;
    if (menuIndex > 2) menuIndex = 0; // Go back to first option
    showMenu();
  }

  if (digitalRead(PIN_BUTTON_2) == LOW) { // Button 2 pressed
    delay(200);
    menuSelected = true; // Mark menu option as selected
  }

  if (millis() - lastUpdate >= updateInterval) { // If time to update weather
    fetchWeatherData();
    lastUpdate = millis();
  }

  if (currentTemps.isNull()) { // If data missing, go back to menu
    showMenu();
  }

  if (menuSelected) { // If user selected something
    if (menuIndex == 0) {
      drawTemperatureGraph(currentTemps); // Show graph
    } else if (menuIndex == 1) {
      showCurrentTemperature();           // Show temperature
    } else if (menuIndex == 2) {
      showWiFiStatus();                    // Show WiFi status
    }
    delay(3000); // Wait 3 seconds
    showMenu(); // Go back to menu
    menuSelected = false;
  }
}

// === Function to draw temperature graph ===
void drawTemperatureGraph(ArduinoJson::JsonArray temps) {
  tft.fillScreen(TFT_BLACK);

  int x0 = 30;
  int y0 = 140;
  int width = 260;
  int height = 100;

  // Draw X and Y axes
  tft.drawLine(x0, y0, x0 + width, y0, TFT_WHITE);
  tft.drawLine(x0, y0, x0, y0 - height, TFT_WHITE);

  // Find min and max temperature
  float minTemp = temps[0].as<float>();
  float maxTemp = temps[0].as<float>();
  for (int i = 0; i < 24; i++) {
    float temp = temps[i].as<float>();
    if (temp < minTemp) minTemp = temp;
    if (temp > maxTemp) maxTemp = temp;
  }

  int stepX = width / 23; // Space between points

  // Draw the temperature lines and dots
  for (int i = 0; i < 23; i++) {
    int x1 = x0 + i * stepX;
    int x2 = x0 + (i + 1) * stepX;

    int y1 = y0 - ((temps[i].as<float>() - minTemp) / (maxTemp - minTemp)) * height;
    int y2 = y0 - ((temps[i + 1].as<float>() - minTemp) / (maxTemp - minTemp)) * height;

    tft.drawLine(x1, y1, x2, y2, TFT_GREEN);
    tft.fillCircle(x1, y1, 2, TFT_RED);
  }

  // Draw last point
  tft.fillCircle(x0 + 23 * stepX, y0 - ((temps[23].as<float>() - minTemp) / (maxTemp - minTemp)) * height, 2, TFT_RED);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  // Draw Y axis temperature labels
  int divisions = 5;
  for (int i = 0; i <= divisions; i++) {
    float value = minTemp + i * (maxTemp - minTemp) / divisions;
    int yPos = y0 - (i * height / divisions);
    tft.drawString(String(value, 1) + "C", 0, yPos - 4);
  }

  // Draw X axis time labels
  tft.drawString("0h", x0, y0 + 10);
  tft.drawString("6h", x0 + stepX * 6, y0 + 10);
  tft.drawString("12h", x0 + stepX * 12, y0 + 10);
  tft.drawString("18h", x0 + stepX * 18, y0 + 10);
  tft.drawString("23h", x0 + stepX * 23 - 10, y0 + 10);
}

// === Show current temperature on screen ===
void showCurrentTemperature() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Location: Karlskrona", 10, 30);
  tft.drawString("Temp: " + String(currentTemperature, 1) + " C", 10, 70);
}

// === Show WiFi status ===
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
