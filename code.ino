#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>

// --- Hardware & WiFi Konfiguration ---
#define TFT_BL 21
const char* ssid     = "DEINE_SSID";
const char* password = "DEIN_PASSWORT";

// --- API & Intervalle ---
const char* weatherUrl = "http://api.brightsky.dev/current_weather?lat=51.48&lon=7.86";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherInterval = 15 * 60 * 1000; // 15 Minuten

// --- Farben (E-Paper Style) ---
#define EPAPER_CREME 0xF7BE
#define EPAPER_BLACK 0x0000

// --- Instanzen ---
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft); // Full Screen Sprite für Double Buffering

// Globaler Wetter-Speicher
float currentTemp = 0.0;
bool weatherLoaded = false;

// --- Hilfsfunktion: CIDR Berechnung ---
String getCIDR(IPAddress subnet) {
  uint32_t mask = (subnet[0] << 24) | (subnet[1] << 16) | (subnet[2] << 8) | (subnet[3]);
  int count = 0;
  while (mask > 0) {
    mask = mask << 1;
    count++;
  }
  return "/" + String(count);
}

// --- Wetter API Abfrage ---
void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getStream());
      currentTemp = doc["weather"]["temperature"];
      weatherLoaded = true;
      Serial.println("Wetter aktualisiert: " + String(currentTemp) + "°C");
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);

  // Display Initialisierung
  tft.init();
  tft.setRotation(1); // Querformat
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Backlight an

  // Sprite Buffer (320x240 @ 16bit = 153.6KB RAM)
  img.createSprite(320, 240);
  img.setTextColor(EPAPER_BLACK, EPAPER_CREME);

  // WiFi & Zeit
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  // Zeitkonfiguration Berlin inkl. Sommerzeit
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "de.pool.ntp.org");

  updateWeather();
}

void drawDisplay() {
  img.fillSprite(EPAPER_CREME);
  
  // --- OBEN: System Info (1/6) ---
  img.setTextDatum(TL_DATUM);
  img.setFreeFont(&FreeSans9pt7b);
  String ipInfo = WiFi.localIP().toString() + getCIDR(WiFi.subnetMask());
  img.drawString(ipInfo, 10, 10);

  // WLAN Balken zeichnen
  int32_t rssi = WiFi.RSSI();
  img.setTextDatum(TR_DATUM);
  img.drawString(String(rssi) + " dBm", 310, 10);
  
  // --- MITTE: Uhrzeit (2/3) ---
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    
    img.setTextDatum(MC_DATUM);
    // Nutze Font 7 für 7-Segment-Optik (Größe 7)
    img.setTextColor(EPAPER_BLACK, EPAPER_CREME);
    img.drawString(timeStr, 160, 110, 7); 
  }

  // --- UNTEN: Datum & Wetter (1/6) ---
  char dateStr[20];
  strftime(dateStr, sizeof(dateStr), "%a, %d. %b", &timeinfo);
  
  img.setTextDatum(BC_DATUM);
  img.setFreeFont(&FreeSans12pt7b);
  img.drawString(dateStr, 160, 190);

  if (weatherLoaded) {
    String tempStr = String(currentTemp, 1) + " C";
    img.setFreeFont(&FreeSansBold18pt7b);
    img.drawString(tempStr, 160, 230);
    // Kleiner Kreis für das Grad-Symbol
    int tempWidth = img.textWidth(tempStr);
    img.drawCircle(160 + (tempWidth/2) - 15, 210, 3, EPAPER_BLACK);
  }

  // Alles auf einmal auf den Screen pushen (Flackerfrei)
  img.pushSprite(0, 0);
}

void loop() {
  // Wetter Timer
  if (millis() - lastWeatherUpdate >= weatherInterval) {
    updateWeather();
    lastWeatherUpdate = millis();
  }

  // Display Refresh (z.B. jede Sekunde)
  drawDisplay();
  delay(1000); 
}
