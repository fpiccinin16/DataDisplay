// --- Libraries ---
#include <Arduino.h>
#include <LittleFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mcp_can.h>
#include <vector>
#include <lvgl.h>

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

// Deklariere die Montserrat-Schriftarten
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_IMG_DECLARE(img_cropped_logo11);


// Enable PSRAM
#if CONFIG_IDF_TARGET_ESP32
  #if CONFIG_SPIRAM_SUPPORT
    #define USE_PSRAM
  #endif
#endif


// --- CAN und TFT Setup ---
TFT_eSPI tft = TFT_eSPI();
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;

// CAN-Modul Pins entsprechend der Tabelle
#define CAN_CS_PIN  21     // CS Pin
#define CAN_INT_PIN 4     // INT Pin (nicht in Verwendung in der Tabelle)
#define CAN_SCK_PIN 13    // SCK Pin
#define CAN_MOSI_PIN 23   // SI (MOSI) Pin
#define CAN_MISO_PIN 19   // SO (MISO) Pin

// Add these definitions after your existing pin definitions
#define DIFF_TEMP_PIN 35    
#define GETRIEBE_TEMP_PIN 32    

// --- Touchscreen Setup ---
TFT_eSPI_Button touchBtn;

// --- Bluetooth Setup ---
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// BLE-Callbacks für Verbindungsstatus
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Bluetooth-Gerät verbunden.");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Bluetooth-Gerät getrennt.");
    }
};

// --- GUI Elemente ---
static lv_obj_t *scr_main;

// Globale Definition von `objects`
struct {
    lv_obj_t *main;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *ubatt_vwert;
    lv_obj_t *wasser_temp_wert;
    lv_obj_t *diff_temp_wert;
    lv_obj_t *getriebe_temp_wert;
    lv_obj_t *oel_druck_wert;
    lv_obj_t *oil_temp_wert;
} objects;

// --- Globale Variablen ---
static lv_obj_t *statusLabel;
static lv_timer_t *hideLabelTimer;
bool hideLabelFlag = false;
unsigned long hideTime = 0;

// Add this function before setup()
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

// Add this before create_screen_main()
void hideLabelCallback(lv_timer_t * timer) {
    if (statusLabel) {
        lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

// Konstanten für NTC-Berechnung

#define NTC_PULLUP 10000.0   // bleibt erlaubt
#define R_ROOM 54000.0
#define T_ROOM 21.0
#define R_HOT 15000.0
#define T_HOT 85.0
#define BETA ((log(R_ROOM/R_HOT))/((1.0/(T_ROOM+273.15))-(1.0/(T_HOT+273.15))))

float readTemperature(int sensorPin) {
    const int numReadings = 10;
    long sum = 0;

    for (int i = 0; i < numReadings; i++) {
        sum += analogRead(sensorPin);
        delay(1);
    }

    int raw = sum / numReadings;
    if (raw <= 0 || raw >= 4095) return -99.9;

    float r_ntc = 10000.0f * (raw / (4095.0f - raw));  // Spannungsteiler NTC oben

    // Steinhart-Hart Koeffizienten (berechnet aus deinen Messpunkten)
    const float A = 0.00361683f;
    const float B = -0.00017981f;
    const float C = 0.000001342f;

    float logR = log(r_ntc);
    float tempK = 1.0f / (A + B * logR + C * pow(logR, 3));
    float tempC = tempK - 273.15f;

    //Serial.printf("Pin %d Raw: %d | R_NTC: %.0f Ω | T: %.1f °C\n", sensorPin, raw, r_ntc, tempC);
    return tempC;
}





void create_screen_main() {
    // Hauptbildschirm erstellen
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, lv_color_black(), LV_PART_MAIN); // Hintergrund schwarz
    lv_obj_set_style_bg_opa(scr_main, LV_OPA_COVER, LV_PART_MAIN);       // Deckkraft auf 100%

    // Canvas hinzufügen
    {
        lv_obj_t *canvas1 = lv_canvas_create(scr_main);
        objects.obj1 = canvas1;
        lv_obj_set_pos(canvas1, 0, 0);
        lv_obj_set_size(canvas1, 160, 80);
        lv_obj_set_style_border_color(canvas1, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(canvas1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        lv_obj_t *canvas2 = lv_canvas_create(scr_main);
        objects.obj2 = canvas2;
        lv_obj_set_pos(canvas2, 0, 80);
        lv_obj_set_size(canvas2, 160, 80);
        lv_obj_set_style_border_color(canvas2, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(canvas2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        lv_obj_t *canvas3 = lv_canvas_create(scr_main);
        objects.obj3 = canvas3;
        lv_obj_set_pos(canvas3, 0, 160);
        lv_obj_set_size(canvas3, 160, 80);
        lv_obj_set_style_border_color(canvas3, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(canvas3, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        lv_obj_t *canvas4 = lv_canvas_create(scr_main);
        objects.obj4 = canvas4;
        lv_obj_set_pos(canvas4, 160, 0);
        lv_obj_set_size(canvas4, 160, 80);
        lv_obj_set_style_border_color(canvas4, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(canvas4, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        lv_obj_t *canvas5 = lv_canvas_create(scr_main);
        objects.obj5 = canvas5;
        lv_obj_set_pos(canvas5, 160, 80);
        lv_obj_set_size(canvas5, 160, 80);
        lv_obj_set_style_border_color(canvas5, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(canvas5, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        lv_obj_t *canvas6 = lv_canvas_create(scr_main);
        objects.obj6 = canvas6;
        lv_obj_set_pos(canvas6, 160, 160);
        lv_obj_set_size(canvas6, 160, 80);
        lv_obj_set_style_border_color(canvas6, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(canvas6, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // Labels hinzufügen
    {
        lv_obj_t *obj = lv_label_create(scr_main);
        objects.obj7 = obj;
        lv_obj_set_pos(obj, 10, 5);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "Oel Temp °C");
    }
    {
        lv_obj_t *obj = lv_label_create(scr_main);
        objects.obj8 = obj;
        lv_obj_set_pos(obj, 172, 5);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "Diff Temp °C");
    }
    {
        lv_obj_t *obj = lv_label_create(scr_main);
        objects.obj9 = obj;
        lv_obj_set_pos(obj, 10, 85);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "Getriebe Temp °C");
    }
    {
        lv_obj_t *obj = lv_label_create(scr_main);
        objects.obj10 = obj;
        lv_obj_set_pos(obj, 172, 87);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "Wasser Temp °C");
    }
    {
        lv_obj_t *obj = lv_label_create(scr_main);
        objects.obj11 = obj;
        lv_obj_set_pos(obj, 12, 167);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "Oel Druck bar");
    }
    {
        lv_obj_t *obj = lv_label_create(scr_main);
        objects.obj12 = obj;
        lv_obj_set_pos(obj, 172, 167);
        lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(obj, "UBatt V");
    }

    // Textarea-Felder hinzufügen
    {
        // UBattVWert
        lv_obj_t *obj = lv_textarea_create(scr_main);
        objects.ubatt_vwert = obj;
        lv_obj_set_pos(obj, 190, 183);
        lv_obj_set_size(obj, 103, 55);
        lv_textarea_set_max_length(obj, 128);
        lv_textarea_set_text(obj, "13.4");
        lv_textarea_set_one_line(obj, false);
        lv_textarea_set_password_mode(obj, false);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT); // Montserrat 32
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // WasserTempWert
        lv_obj_t *obj = lv_textarea_create(scr_main);
        objects.wasser_temp_wert = obj;
        lv_obj_set_pos(obj, 190, 103);
        lv_obj_set_size(obj, 103, 55);
        lv_textarea_set_max_length(obj, 128);
        lv_textarea_set_text(obj, "110");
        lv_textarea_set_one_line(obj, false);
        lv_textarea_set_password_mode(obj, false);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT); // Montserrat 32
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // DiffTempWert
        lv_obj_t *obj = lv_textarea_create(scr_main);
        objects.diff_temp_wert = obj;
        lv_obj_set_pos(obj, 190, 21);
        lv_obj_set_size(obj, 103, 55);
        lv_textarea_set_max_length(obj, 128);
        lv_textarea_set_text(obj, "78");
        lv_textarea_set_one_line(obj, false);
        lv_textarea_set_password_mode(obj, false);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT); // Verwenden Sie das richtige Schriftart-Objekt
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // GetriebeTempWert
        lv_obj_t *obj = lv_textarea_create(scr_main);
        objects.getriebe_temp_wert = obj;
        lv_obj_set_pos(obj, 29, 100);
        lv_obj_set_size(obj, 103, 55);
        lv_textarea_set_max_length(obj, 128);
        lv_textarea_set_text(obj, "98");
        lv_textarea_set_one_line(obj, false);
        lv_textarea_set_password_mode(obj, false);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // OelDruckWert
        lv_obj_t *obj = lv_textarea_create(scr_main);
        objects.oel_druck_wert = obj;
        lv_obj_set_pos(obj, 29, 183);
        lv_obj_set_size(obj, 103, 55);
        lv_textarea_set_max_length(obj, 128);
        lv_textarea_set_text(obj, "7");
        lv_textarea_set_one_line(obj, false);
        lv_textarea_set_password_mode(obj, false);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    {
        // OilTempWert
        lv_obj_t *obj = lv_textarea_create(scr_main);
        objects.oil_temp_wert = obj;
        lv_obj_set_pos(obj, 30, 21);
        lv_obj_set_size(obj, 103, 55);
        lv_textarea_set_max_length(obj, 128);
        lv_textarea_set_text(obj, "112");
        lv_textarea_set_one_line(obj, false);
        lv_textarea_set_password_mode(obj, false);
        lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(obj, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // Status Label erstellen und konfigurieren
    statusLabel = lv_label_create(scr_main);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xFD20), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(statusLabel, LV_OBJ_FLAG_HIDDEN); // Initial verstecken
    
    // Timer für das Ausblenden erstellen
    hideLabelTimer = lv_timer_create(hideLabelCallback, 5000, NULL);
    lv_timer_pause(hideLabelTimer);

    // Hauptbildschirm laden
    lv_scr_load(scr_main);
}

// Nach den globalen Variablen, vor setup():
MCP_CAN CAN(CAN_CS_PIN);  // CAN-Objekt erstellen
bool canInitialized = false;

// CAN-Initialisierungsfunktion
bool initializeCAN() {
    // SPI für CAN-Modul konfigurieren
    SPI.begin(CAN_SCK_PIN, CAN_MISO_PIN, CAN_MOSI_PIN, CAN_CS_PIN);
    
    Serial.println("Versuche CAN-Modul zu initialisieren...");
    
    for(int i = 0; i < 3; i++) {
        // Versuche Initialisierung mit STDEXT-Modus und 500kbps
        if(CAN.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
            Serial.println("MCP2515 Initialized Successfully!");
            
            // Konfigurationsmodus setzen
            if(CAN.setMode(MCP_LISTENONLY) == CAN_OK) {
                Serial.println("MCP2515 in Listen Only Mode");
                
                // Alle Masken und Filter deaktivieren
                CAN.init_Mask(0, false, 0x00000000);
                CAN.init_Mask(1, false, 0x00000000);
                
                for(int i = 0; i < 6; i++) {
                    CAN.init_Filt(i, false, 0x00000000);
                }
                
                // In den normalen Modus wechseln
                if(CAN.setMode(MCP_NORMAL) == CAN_OK) {
                    Serial.println("MCP2515 is now in Normal Mode");
                    return true;
                }
            }
        }
        Serial.println("Initialisierungsversuch fehlgeschlagen, versuche erneut...");
        delay(1000);
    }
    
    Serial.println("CAN-Modul konnte nicht initialisiert werden!");
    return false;
}

void setup() {
    // Serielle Kommunikation starten
    Serial.begin(115200);
    Serial.println("Setup wird gestartet...");

    // LVGL initialisieren
    lv_init();

    // Display initialisieren
    tft.begin();
    tft.setRotation(1);

    // Speicher für den Display-Puffer zuweisen
    buf1 = (lv_color_t *)ps_malloc(LV_HOR_RES_MAX * LV_VER_RES_MAX * sizeof(lv_color_t));
    if (!buf1) {
        Serial.println("Fehler: Puffer konnte nicht zugewiesen werden!");
        while (1); // Endlosschleife bei Fehler
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);

    // Display-Treiber initialisieren
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Hauptbildschirm erstellen
    create_screen_main();
    lv_scr_load(scr_main);

    // ADC Konfiguration 
    analogReadResolution(12);
    pinMode(DIFF_TEMP_PIN, INPUT);      // Kein Pull-up
    pinMode(GETRIEBE_TEMP_PIN, INPUT);  // Kein Pull-up

    // CAN-Modul initialisieren
    canInitialized = initializeCAN();
    if (canInitialized) {
        Serial.println("CAN-Bus bereit!");
    } else {
        Serial.println("CAN-Bus Fehler!");
    }
}

void loop() {
    lv_timer_handler();

    // Analoge Sensoren auslesen und Display aktualisieren
    static unsigned long lastAnalogRead = 0;
    if (millis() - lastAnalogRead >= 1000) {  // Alle 1 Sekunde aktualisieren
        // Differential-Temperatur
        float diffTemp = readTemperature(DIFF_TEMP_PIN);
        char diffTempStr[8];
        snprintf(diffTempStr, sizeof(diffTempStr), "%.1f", diffTemp);
        lv_textarea_set_text(objects.diff_temp_wert, diffTempStr);
        
        // Getriebe-Temperatur
        float getriebeTemp = readTemperature(GETRIEBE_TEMP_PIN);
        char getriebeTempStr[8];
        snprintf(getriebeTempStr, sizeof(getriebeTempStr), "%.1f", getriebeTemp);
        lv_textarea_set_text(objects.getriebe_temp_wert, getriebeTempStr);
        
        // Debug output
        //Serial.printf("Diff Temp: %.1f°C | Getriebe Temp: %.1f°C\n", 
        //             diffTemp, getriebeTemp);
        
        //lastAnalogRead = millis();
    }

    // Your existing CAN code
    if (canInitialized) {
        unsigned long canId;
        unsigned char len = 0;
        unsigned char buf[8];

        if (CAN_MSGAVAIL == CAN.checkReceive()) {
            // Erweiterte Debug-Ausgabe
            if (CAN.readMsgBuf(&canId, &len, buf) == CAN_OK) {
                Serial.printf("CAN ID: 0x%03X Len: %d Data: ", canId, len);
                for(int i = 0; i < len; i++) {
                    Serial.printf("%02X ", buf[i]);
                }
                Serial.println();
            }
        }
    }
    
    delay(5); // Kürzeres Delay für schnellere Reaktion
}

