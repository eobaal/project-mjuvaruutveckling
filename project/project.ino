#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Abo Hasan";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Ansluter till WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi ansluten!");

  HTTPClient http;
  http.begin("http://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/15.5869/lat/56.1612/data.json");
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  Serial.print("HTTP-kod: ");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Här är svaret från SMHI:");
    Serial.println(payload);   // ⚡⚡⚡ Här skriver vi ut ALLT ⚡⚡⚡
  } else {
    Serial.println("Kunde inte hämta från SMHI.");
  }

  http.end();
}

void loop() {
  // Tom
}
