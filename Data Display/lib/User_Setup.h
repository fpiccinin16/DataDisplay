// --- Display Driver ---
#define ILI9341_DRIVER

// --- Display Pins ---
#define TFT_CS   5    // Chip select control pin
#define TFT_DC   2    // Data Command control pin
#define TFT_RST  4    // Reset pin (could connect to RST pin)

// --- SPI Pins ---
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO 19

// --- SPI Settings ---
#define SPI_FREQUENCY  40000000  // 40MHz für schnelles Display
#define SPI_READ_FREQUENCY 20000000

// --- Touch Settings ---
#define TOUCH_CS   15     // Chip select pin for the touch controller
#define TOUCH_IRQ  27     // Optional, wenn du IRQ nutzen willst

// --- Touch SPI Frequenz ---
#define SPI_TOUCH_FREQUENCY 2500000 // 2.5 MHz für Touch stabil

// Optionale weitere Optimierungen für ESP32
#define SUPPORT_TRANSACTIONS
