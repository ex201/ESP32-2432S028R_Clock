#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>

// --- Hardware Pins (CYD / ESP32-2432S028R) ---
#define TFT_BL 21

// --- WiFi Konfiguration ---
const char* ssid     = "WLAN_SSID";
const char* password = "WLAN_KENNWORT";

TFT_eSPI tft = TFT_eSPI();
// Wir nutzen ein kleineres Sprite NUR für die Uhrzeit, um RAM zu sparen
TFT_eSprite timeSprite = TFT_eSprite(&tft); 

float currentTemp = 0.0;
bool weatherValid = false;
unsigned long lastWeatherUpdate = 0;
unsigned long lastRssiUpdate = 0;

// --- 1. Hilfsfunktionen ---

String getCIDR(IPAddress subnet) {
  uint32_t mask = (subnet[0] << 24) | (subnet[1] << 16) | (subnet[2] << 8) | subnet[3];
  int prefix = 0;
  for (int i = 0; i < 32; i++) if ((mask << i) & 0x80000000) prefix++;
  return "/" + String(prefix);
}

void drawWiFiSignal(int x, int y) {
  // Bereich oben rechts säubern (damit es keine "Reste" gibt)
  // Anpassbar, falls du es enger/weiter haben willst
  tft.fillRect(170, 0, 150, 35, 0xF7BE);
  int32_t rssi = WiFi.RSSI();
  int filledBars = 0;
  if (rssi > -55) filledBars = 4;
  else if (rssi > -65) filledBars = 3;
  else if (rssi > -75) filledBars = 2;
  else if (rssi > -85) filledBars = 1;

  // Textwert links daneben
  tft.setTextDatum(CR_DATUM);
  tft.setTextColor(TFT_BLACK, 0xF7BE);
  tft.drawString(String(rssi) + " dBm", x - 10, y + 8, 2);

  // 4 Balken
  for (int i = 0; i < 4; i++) {
    int h = (i + 1) * 4;
    int bx = x + (i * 6);
    int by = y + (16 - h);
    tft.drawRect(bx, by, 4, h, TFT_BLACK); // Umrandung
    if (i < filledBars) {
      tft.fillRect(bx, by, 4, h, TFT_BLACK); // Füllung
    } else {
      tft.fillRect(bx + 1, by + 1, 2, h - 2, 0xF7BE); // Leeren
    }
  }
}

void updateWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin("http://api.brightsky.dev/current_weather?lat=51.48&lon=7.86");
  if (http.GET() == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getStream());
    currentTemp = doc["weather"]["temperature"];
    weatherValid = true;
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  
  // Backlight sofort erzwingen
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(0xF7BE); // EPAPER_CREME

  // Kleines Sprite für die Uhr (ca. 200x60 Pixel spart massiv RAM)
  if (!timeSprite.createSprite(280, 90)) {
    Serial.println("Sprite Fehler!");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "de.pool.ntp.org");
  updateWeather();
  
  // Einmaliges Zeichnen des statischen Layouts
  drawStaticLayout();
}

// --- 2. Haupt-Anzeige-Logik ---

void drawStaticLayout() {
  tft.setTextColor(TFT_BLACK, 0xF7BE);
  tft.drawFastHLine(0, 35, 320, TFT_BLACK);
  tft.drawFastHLine(0, 185, 320, TFT_BLACK);
  
  // Oben links: IP Info (Statisch)
  tft.setTextDatum(TL_DATUM);
  IPAddress ip = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();
  String cidr = getCIDR(subnet);

  tft.drawString(ip.toString() + cidr, 10, 10, 2);
  drawWiFiSignal(270, 10);
}


void loop() {
  if (millis() - lastWeatherUpdate > 900000) {
    updateWeather();
    lastWeatherUpdate = millis();
    drawDynamicData(); // Wetter-Update auf Screen
  }

  static int lastMin = -1;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (timeinfo.tm_min != lastMin) {
      lastMin = timeinfo.tm_min;
      drawDynamicData();
    }
  }
  // WLAN Signalstärke alle 10 Sekunden aktualisieren
  if (millis() - lastRssiUpdate > 10000) {
  lastRssiUpdate = millis();
  drawWiFiSignal(270, 10); // Event auch 260 anstatt 270 falls zu weit rechts
  }
  delay(1000);
}

void drawDynamicData() {
  // --- 1. UHRZEIT (Sprite) ---
  timeSprite.fillSprite(0xF7BE);
  timeSprite.setTextColor(TFT_BLACK, 0xF7BE);
  
  char timeStr[6];
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return; // Sicherheitshalber abbrechen wenn Zeit nicht da
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);

  timeSprite.setTextDatum(MC_DATUM);
  timeSprite.setTextSize(2); // Doppelte Größe für Font 7
  timeSprite.drawString(timeStr, 140, 45, 7); 
  timeSprite.pushSprite(20, 65); // Positioniert die große Uhr mittig

  // --- 2. FUSSZEILE: Datum & Wetter nebeneinander ---
  // Bereich säubern
  tft.fillRect(0, 187, 320, 53, 0xF7BE); 
  tft.setTextColor(TFT_BLACK, 0xF7BE);

  // Datum (Links-Bündig, Mitte der Zeile)
  char dateStr[20];
  strftime(dateStr, sizeof(dateStr), "%a, %d.%m.", &timeinfo);
  tft.setTextDatum(CL_DATUM); // Centre Left
  tft.drawString(dateStr, 20, 215, 4); 

  // --- 3. TEMPERATUR (Rechts-Bündig, Mitte der Zeile) ---
  if (weatherValid) {
    String tempStr = String(currentTemp, 1) + "  C"; // Zwei Leerzeichen vor dem C für das Symbol
    tft.setTextDatum(CR_DATUM); // Centre Right
    tft.drawString(tempStr, 305, 215, 4);

    // Präzise Berechnung für das Grad-Symbol:
    // Wir messen die Breite des " C" Teils, um den Kreis davor zu setzen
    int cWidth = tft.textWidth(" C", 4);
    int totalWidth = tft.textWidth(tempStr, 4);
    
    // Position: Rechte Kante (305) minus ein Teil der Breite
    int circleX = 305 - cWidth + 2; 
    tft.drawCircle(circleX, 204, 2, TFT_BLACK); 
  }
}
