
## Bibliotheken
Stelle sicher, dass du die Bibliotheken  **TFT_eSPI**,  **ArduinoJson**  und die Standard-ESP32-Bibliotheken installiert hast. Da das CYD spezifische Pins nutzt, muss deine  `User_Setup.h`  in der TFT_eSPI Bibliothek korrekt auf das  **ILI9341**  (oder ST7789, je nach Version) konfiguriert sein.

## Erläuterungen zur Implementierung

-   **Double Buffering:**  Das gesamte Bild wird zuerst im RAM (im  `img`  Objekt) gezeichnet. Erst der Befehl  `img.pushSprite(0, 0)`  kopiert den fertigen Speicherinhalt in Millisekunden auf das Display. Das verhindert das typische "Zeilenweise Aufbauen" oder Flackern.
-   **CIDR-Logik:**  Die Funktion  `getCIDR`  wandelt die Subnetzmaske (meist  `255.255.255.0`) in die  `/24`  Notation um, indem sie die gesetzten Bits zählt.
-   **Zeit & Sommerzeit:**  Durch  `setenv`  bzw.  `configTzTime`  mit dem Parameter  `CET-1CEST,M3.5.0,M10.5.0/3`  übernimmt die ESP32-interne Library die Umstellung von Sommer- auf Winterzeit am letzten Sonntag im März bzw. Oktober vollautomatisch.
-   **7-Segment Font:**  `img.drawString(timeStr, 160, 110, 7);`  nutzt die in  `TFT_eSPI`  eingebaute Schriftart Nr. 7. Diese ist speziell auf digitale Ziffern optimiert.
-   **E-Paper Ästhetik:**  Die Farbe  `0xF7BE`  ist ein warmes Cremeweiß, das zusammen mit schwarzer Schrift den Kontrast eines E-Ink Displays imitiert, ohne dessen langsame Refresh-Raten zu haben

## ToDo

 - Das CYD hat einen Lichtsensor auf der Rückseite
	 - Gehäuse drucken das ausreichend Licht an diesen lässt
	 - Quellcode anpassen auf Lichtsensor - alternativ fest Uhrzeit definieren für das Auf-/Abblenden bzw. Nachtschaltung
 - `canvas.drawCircle(x, y, 2, EPAPER_BLACK)`
