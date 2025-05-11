#include <WiFi.h>                // Inkluderar bibliotek för WiFi-anslutning
#include <HTTPClient.h>          // Inkluderar bibliotek för att göra HTTP-förfrågningar
#include <ArduinoJson.h>         // Används för att hantera JSON-data från väder-API:t
#include <TFT_eSPI.h>            // Bibliotek för att kontrollera TFT-skärmen
#include "pin_config.h"           // Inkluderar definitioner för knappar
#include <algorithm>             // Inkluderar funktioner som min_element och max_element

// === Funktionsdeklarationer ===
void drawTemperatureGraph();                    // Ritar temperaturgrafen
void showCurrentTemperature();                  // Visar aktuell temperatur på skärmen
void showWiFiStatus();                          // Visar WiFi-status
void fetchWeatherData();                        // Hämtar väderdata från API
void showMenu();                                // Visar meny på skärmen
void drawWeatherIcon(int x, int y, int code);   // Ritar väderikon baserat på väderkod

// === WiFi-inställningar ===
const char* ssid = "Abo Hasan";                  // WiFi-namn (SSID)
const char* password = "12345678";              // WiFi-lösenord

TFT_eSPI tft = TFT_eSPI();                      // Skapar skärmobjekt från TFT_eSPI-biblioteket

// === Menyrelaterade variabler ===
String menuItems[] = {"Weather", "Temperature", "WiFi Status"};  // Menyval
int menuIndex = 0;                              // Visar vilket menyval som är aktivt
bool menuSelected = false;                      // Markerar om ett menyval har gjorts
bool cityChangeRequested = false;               // Markerar om stad ändrats

// === Strukturen för städer ===
struct City {
  const char* name;     // Stadens namn
  float lat;            // Latitud
  float lon;            // Longitud
};

// === Lista över städer att välja mellan ===
City cities[] = {
  {"Karlskrona", 56.1612, 15.5869},
  {"Stockholm", 59.3293, 18.0686},
  {"Goteborg", 57.7089, 11.9746},
  {"Malmo", 55.604981, 13.003822}
};

int cityIndex = 0;  // Index för aktuell stad i listan ovan

// === Datarelaterade variabler ===
DynamicJsonDocument doc(30000);           // JSON-dokument för att spara API-data (30 kB maxstorlek)
std::vector<float> temperatures24h;       // Lista med temperaturer för de kommande 24 timmarna
float currentTemperature = NAN;           // Nuvarande temperatur
int currentWeatherCode = -1;              // Väderkod från API:t

// === Timer för automatisk uppdatering ===
unsigned long lastUpdate = 0;                      // När senaste uppdateringen skedde
const unsigned long updateInterval = 10 * 60 * 1000; // Hur ofta vi ska uppdatera (var 10:e minut)

// === setup() körs en gång vid uppstart ===
void setup() {
  Serial.begin(115200);           // Startar seriell kommunikation för felsökning
  tft.init();                     // Initierar skärmen
  tft.setRotation(1);             // Sätter skärmen till liggande läge
  tft.fillScreen(TFT_BLACK);     // Rensar skärmen till svart bakgrund

  pinMode(PIN_BUTTON_1, INPUT_PULLUP); // Knapparna definieras som ingångar med pull-up
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);

  // Visar välkomsttext
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Hello students", 10, 10);
  delay(2000);
  tft.drawString("Group 9", 10, 40);
  delay(3000);

  // Försöker ansluta till WiFi
  WiFi.begin(ssid, password);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Connecting WiFi...", 10, 10);

  // Väntar max 15 sekunder på att ansluta till WiFi
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  // Om WiFi inte ansluter
  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("WiFi Failed", 10, 50);
    Serial.println("WiFi Failed!");
    while (true); // Stannar programmet
  }

  // Om WiFi ansluter
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WiFi Connected!", 10, 10);
  Serial.println("WiFi Connected!");
  delay(2000);

  fetchWeatherData();  // Hämtar väderdata från API
  showMenu();           // Visar huvudmenyn
  lastUpdate = millis(); // Sparar tiden då vi hämtade data
}

void fetchWeatherData() {
  City city = cities[cityIndex]; // Hämta aktuell stad beroende på användarens val
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(city.lat) + "&longitude=" + String(city.lon) + "&hourly=temperature_2m&current_weather=true"; // Bygg API URL

  HTTPClient http;
  http.begin(url);              // Initierar HTTP-klient med URL
  int httpCode = http.GET();   // Skickar GET-förfrågan till API

  if (httpCode == 200) {       // Om servern svarar OK (200)
    String payload = http.getString();  // Läser svaret (JSON-data)
    DeserializationError error = deserializeJson(doc, payload); // Försöker tolka JSON-datan

    if (!error) { // Om JSON kunde tolkas
      JsonArray temps = doc["hourly"]["temperature_2m"]; // Hämtar temperaturerna för 24 timmar
      temperatures24h.clear(); // Rensa tidigare värden
      for (int i = 0; i < 24; i++) {
        temperatures24h.push_back(temps[i]); // Spara varje temperatur i vektorn
      }
      currentTemperature = doc["current_weather"]["temperature"].as<float>(); // Nuvarande temperatur
      currentWeatherCode = doc["current_weather"]["weathercode"].as<int>();   // Nuvarande väderkod (ikon)
    }
  }
  http.end(); // Avslutar HTTP-förfrågan
}

// === Menyfunktion ===
void showMenu() {
  tft.fillScreen(TFT_BLACK);  // Rensa skärmen
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Gul text
  tft.setTextSize(2);
  tft.drawString("Menu:", 10, 10); // Menyrubrik
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(menuItems[menuIndex], 10, 50); // Aktuellt menyval
  tft.drawString("City: " + String(cities[cityIndex].name), 10, 80); // Visar stad
  tft.drawString("Press to select", 10, 120); // Instruktion
}

// === Huvudloopen som körs om och om igen ===
void loop() {
  if (!menuSelected) { // Om inget menyval är valt ännu
    if (digitalRead(PIN_BUTTON_1) == LOW && digitalRead(PIN_BUTTON_2) == LOW) {
      delay(500);
      cityIndex++; // Byt stad
      if (cityIndex >= 4) cityIndex = 0; // Om vi går förbi sista, börja om
      fetchWeatherData(); // Hämta väderdata för ny stad
      showMenu();         // Visa uppdaterad meny
    }
    else if (digitalRead(PIN_BUTTON_1) == LOW) { // Bläddra i meny
      delay(200);
      menuIndex++;
      if (menuIndex > 2) menuIndex = 0;
      showMenu();
    } else if (digitalRead(PIN_BUTTON_2) == LOW) { // Välj menyval
      delay(200);
      menuSelected = true;
    }
  }

  if (millis() - lastUpdate >= updateInterval) { // Om det gått mer än 10 min
    fetchWeatherData(); // Uppdatera väder
    lastUpdate = millis();
  }

  if (menuSelected) { // Om användaren har valt ett menyval
    if (menuIndex == 0) {
      drawTemperatureGraph(); // Visa vädergraf
    } else if (menuIndex == 1) {
      showCurrentTemperature(); // Visa temperatur
    } else if (menuIndex == 2) {
      showWiFiStatus(); // Visa WiFi-status
    }
    // Vänta tills båda knapparna trycks för att återgå till meny
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

// Ritar temperaturgrafen med Y-axel för temperatur och X-axel för tid
void drawTemperatureGraph() {
  tft.fillScreen(TFT_BLACK); // Rensar skärmen

  int x0 = 30, y0 = 140, width = 260, height = 100; // Ursprung och storlek för grafen
  tft.drawLine(x0, y0, x0 + width, y0, TFT_WHITE);  // Rita X-axel
  tft.drawLine(x0, y0, x0, y0 - height, TFT_WHITE); // Rita Y-axel

  if (temperatures24h.empty()) { // Kontroll om ingen temperaturdata finns
    tft.drawString("No data", 50, 100);
    return;
  }

  float minTemp = *min_element(temperatures24h.begin(), temperatures24h.end()); // Min temperatur
  float maxTemp = *max_element(temperatures24h.begin(), temperatures24h.end()); // Max temperatur

  int stepX = width / 23; // Avstånd mellan punkter längs X-axeln
  for (int i = 0; i < 23; i++) {
    int x1 = x0 + i * stepX;
    int x2 = x0 + (i + 1) * stepX;
    // Skalar temperaturen till höjd på skärmen (y-led)
    int y1 = y0 - ((temperatures24h[i] - minTemp) / (maxTemp - minTemp)) * height;
    int y2 = y0 - ((temperatures24h[i+1] - minTemp) / (maxTemp - minTemp)) * height;

    tft.drawLine(x1, y1, x2, y2, TFT_GREEN); // Rita linje mellan punkterna
    tft.fillCircle(x1, y1, 2, TFT_RED);      // Rita en punkt
  }
  // Rita sista punkten
  tft.fillCircle(x0 + 23 * stepX, y0 - ((temperatures24h[23] - minTemp) / (maxTemp - minTemp)) * height, 2, TFT_RED);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);

  // Rita temperaturvärden på Y-axeln
  for (int i = 0; i <= 5; i++) {
    float value = minTemp + i * (maxTemp - minTemp) / 5;
    int yPos = y0 - (i * height / 5);
    tft.drawString(String(value, 1) + "C", 0, yPos - 4);
  }

  // Tidsangivelser på X-axeln
  tft.drawString("0h", x0, y0 + 10);
  tft.drawString("6h", x0 + stepX * 6, y0 + 10);
  tft.drawString("12h", x0 + stepX * 12, y0 + 10);
  tft.drawString("18h", x0 + stepX * 18, y0 + 10);
  tft.drawString("23h", x0 + stepX * 23 - 10, y0 + 10);
}

// Ritar en ikon baserat på väderkod från API:t
void drawWeatherIcon(int x, int y, int code) {
  if (code == 0) { // Sol
    tft.fillCircle(x, y, 10, TFT_YELLOW);
  } else if (code >= 1 && code <= 3) { // Molnigt
    tft.fillCircle(x, y, 10, TFT_LIGHTGREY);
    tft.fillCircle(x + 10, y, 10, TFT_LIGHTGREY);
    tft.fillCircle(x + 5, y - 5, 10, TFT_LIGHTGREY);
  } else if (code >= 51 && code <= 67) { // Regn
    tft.fillCircle(x-5, y, 3, TFT_BLUE);
    tft.fillCircle(x+5, y, 3, TFT_BLUE);
    tft.fillCircle(x, y+10, 3, TFT_BLUE);
  } else if (code >= 95) { // Åska
    tft.drawLine(x, y, x+5, y+10, TFT_YELLOW);
    tft.drawLine(x+5, y+10, x, y+20, TFT_YELLOW);
  } else { // Okänt väder
    tft.drawString("?", x, y, 2);
  }
}

// Visar aktuell temperatur och stad
void showCurrentTemperature() {
  tft.fillScreen(TFT_BLACK);                      // Rensa skärmen
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Location: " + String(cities[cityIndex].name), 10, 30); // Visa stad
  tft.drawString("Temp: " + String(currentTemperature, 1) + " C", 10, 70); // Visa temperatur
  drawWeatherIcon(250, 50, currentWeatherCode);   // Rita väderikon
}

// Visar WiFi-status på skärmen
void showWiFiStatus() {
  tft.fillScreen(TFT_BLACK);            // Rensa skärmen
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString("WiFi: OK", 10, 50); // Ansluten
  } else {
    tft.drawString("WiFi: LOST", 10, 50); // Inte ansluten
  }
}
