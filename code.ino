#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>

// --- Hardware Pins (CYD / ESP32-2432S028R) ---
#define TFT_BL 21

// --- WiFi Konfiguration ---
const char* ssid     = "DEINE_SSID";
const char* password = "DEIN_PASSWORT";

// --- API & Intervalle ---
const char* weatherUrl = "http://api.brightsky.dev/current_weather?lat=51.48&lon=7.86";
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherInterval = 15 * 60 * 1000; // 15 Min

// --- Design (E-Paper Palette) ---
#define EPAPER_CREME 0xF7BE // Warmer Hintergrund
#define EPAPER_BLACK 0x0000 // Schrift & Linien

// --- Objekte ---
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft); // Double Buffer Sprite

// Globaler Status
float currentTemp = 0.0;
bool weatherValid = false;

// --- Hilfsfunktion: Subnetz zu CIDR ---
String getCIDR(IPAddress subnet) {
  uint32_t mask = (subnet[0] << 24) | (subnet[1] << 16) | (subnet[2] << 8) | subnet[3];
  int prefix = 0;
  for (int i = 0; i < 32; i++) {
    if ((mask << i) & 0x80000000) prefix++;
  }
  return "/" + String(prefix);
}

// --- Optimierter Wetter-Abruf mit JSON-Filter ---
void updateWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(weatherUrl);
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    // Filter erstellen: Nur "weather" -> "temperature" interessiert uns
    JsonDocument filter;
    filter["weather"]["temperature"] = true;

    JsonDocument doc;
    deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    
    if (doc["weather"].containsKey("temperature")) {
      currentTemp = doc["weather"]["temperature"];
      weatherValid = true;
      Serial.printf("Wetter Update: %.1f °C\n", currentTemp);
    }
  } else {
    Serial.printf("HTTP Fehler: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void setup() {
  Serial.begin(115200);

  // 1. Display Hardware Setup
  tft.init();
  tft.setRotation(1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 2. Sprite initialisieren (320x240 Pixel, 16-bit Farbe)
  if (!canvas.createSprite(320, 240)) {
    Serial.println("Sprite Fehler! Nicht genug RAM.");
    while(1);
  }

  // 3. Verbindung & Zeit
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // Berlin Timezone mit automatischer Sommerzeit
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "de.pool.ntp.org");

  updateWeather();
}

void drawDisplay() {
  // Hintergrund löschen
  canvas.fillSprite(EPAPER_CREME);
  canvas.setTextColor(EPAPER_BLACK, EPAPER_CREME);

  // --- TOP SECTION: Network ---
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextFont(2); // Kleine Standard-Schrift
  String netInfo = WiFi.localIP().toString() + getCIDR(WiFi.subnetMask());
  canvas.drawString(netInfo, 10, 8);

  // WLAN Signalstärke
  int rssi = WiFi.RSSI();
  canvas.setTextDatum(TR_DATUM);
  canvas.drawString(String(rssi) + " dBm", 310, 8);
  
  // Design-Linie Oben
  canvas.drawFastHLine(0, 35, 320, EPAPER_BLACK);

  // --- MIDDLE SECTION: Clock ---
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[10];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    
    canvas.setTextDatum(MC_DATUM);
    // Font 7 ist der 7-Segment Font (Höhe ca. 48px)
    canvas.drawString(timeStr, 160, 110, 7); 
  }

  // Design-Linie Unten
  canvas.drawFastHLine(0, 180, 320, EPAPER_BLACK);

  // --- BOTTOM SECTION: Date & Weather ---
  char dateStr[32];
  // Wochentag kurz + Tag + Monat
  strftime(dateStr, sizeof(dateStr), "%a, %d. %b", &timeinfo);
  
  canvas.setTextDatum(BC_DATUM);
  canvas.setTextFont(4); // Mittlere klare Schrift
  canvas.drawString(dateStr, 160, 208);

  if (weatherValid) {
    // Temperatur groß
    canvas.setTextDatum(BC_DATUM);
    String t = String(currentTemp, 1) + " 'C"; // ' wird im Font oft als Grad genutzt oder händisch
    canvas.setTextFont(4);
    canvas.drawString(t, 160, 235);
  }

  // --- Push to Screen ---
  canvas.pushSprite(0, 0);
}

void loop() {
  // Nicht-blockierender Wetter-Update
  if (millis() - lastWeatherUpdate >= weatherInterval) {
    updateWeather();
    lastWeatherUpdate = millis();
  }

  // Display-Refresh alle 1 Sekunde für die Uhrzeit
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw >= 1000) {
    drawDisplay();
    lastDraw = millis();
  }
}
