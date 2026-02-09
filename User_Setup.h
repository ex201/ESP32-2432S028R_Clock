// ###### KONFIGURATION FÜR ESP32-2432S028R (CYD) ######

#define USER_SETUP_LOADED

// 1. Treiber
#define ILI9341_2_DRIVER     // Spezielle Variante für das CYD

// 2. Display-Pins
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1          // Reset ist am ESP32-Reset gekoppelt
#define TFT_BL   21          // Backlight Pin
#define TFT_BACKLIGHT_ON HIGH

// 3. Touch-Pins (Falls du später Touch nutzen willst)
#define TOUCH_CS 33

// 4. Schriftarten (Fonts)
#define LOAD_GLCD   // Standard Font
#define LOAD_FONT2  // Kleine Schrift
#define LOAD_FONT4  // Mittlere Schrift
#define LOAD_FONT6  // Große Ziffern
#define LOAD_FONT7  // 7-Segment Font (Wichtig für dein Projekt!)
#define LOAD_FONT8  // Riesige Ziffern
#define LOAD_GFXFF  // FreeFonts Support
#define SMOOTH_FONT

// 5. SPI Frequenz
#define SPI_FREQUENCY  55000000 // 55MHz für flüssiges Sprite-Pushing
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
