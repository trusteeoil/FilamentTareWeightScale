// ============================================================
//  Filament Tare Scale
//  Hardware: Elegoo ESP32 + HX711 + SSD1306 128x64 OLED
// ============================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

// --- HX711 pins ---
#define HX711_DT   4
#define HX711_SCK  5

// --- OLED ---
#define OLED_WIDTH  128
#define OLED_HEIGHT  64
#define OLED_ADDR   0x3C

// --- Calibration ---
#define CALIBRATION_FACTOR  370.0f

// --- Tare weights in grams ---
const float TARE_BAMBU_G      = 255.0f;
const float TARE_CARDBOARD_G  = 150.0f;
const float TARE_PLASTIC_G    = 165.0f;

// --- Averaging ---
#define NUM_SAMPLES 8

// --- Display sleep ---
#define SLEEP_TIMEOUT_MS  (3UL * 60UL * 1000UL)  // 3 minutes
#define WAKE_THRESHOLD_G  10.0f                   // grams of change to wake

// ============================================================

HX711 scale;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

bool    displayOn       = true;
float   idleWeight      = 0.0f;   // gross weight when screen last went idle
unsigned long lastChangeMs = 0;

// ============================================================
void setup() {
    Serial.begin(115200);

    scale.begin(HX711_DT, HX711_SCK);
    scale.set_scale(CALIBRATION_FACTOR);
    scale.tare();

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 init failed");
        for (;;) {}
    }
    display.clearDisplay();
    display.display();

    lastChangeMs = millis();
}

// ------------------------------------------------------------
// Each row is 16px tall (size-2 number height).
// Label is size 1 (8px), vertically centered in the row (+4px offset).
// Number is size 2, right-aligned.
// ------------------------------------------------------------
static void drawRow(int y, const char* label, float netGrams) {
    // Label — size 1, nudged down to vertically center in 16px row
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, y + 4);
    display.print(label);

    // Number — size 2, right-aligned
    // Use 5-char field so "---" and "212g" sit at the same x position
    char buf[12];
    if (netGrams < 0.0f) {
        strcpy(buf, "  ---");
    } else {
        int gInt = (int)(netGrams + 0.5f);
        snprintf(buf, sizeof(buf), "%4dg", gInt);
    }

    int textW = strlen(buf) * 12;  // size 2: 12px per char
    display.setTextSize(2);
    display.setCursor(OLED_WIDTH - textW, y);
    display.print(buf);
}

// ============================================================
void loop() {
    if (!scale.is_ready()) return;

    float grossGrams = scale.get_units(NUM_SAMPLES);

    // --- Sleep / wake logic ---
    if (!displayOn) {
        // Wake if weight has shifted significantly from when we slept
        if (fabsf(grossGrams - idleWeight) >= WAKE_THRESHOLD_G) {
            display.ssd1306_command(SSD1306_DISPLAYON);
            displayOn    = true;
            lastChangeMs = millis();
        } else {
            return;  // screen off, nothing changed — skip redraw
        }
    } else {
        if (fabsf(grossGrams - idleWeight) >= WAKE_THRESHOLD_G) {
            // Weight is moving — reset the idle timer and update baseline
            idleWeight   = grossGrams;
            lastChangeMs = millis();
        } else if (millis() - lastChangeMs >= SLEEP_TIMEOUT_MS) {
            // Stable for 3 minutes — sleep the display
            idleWeight = grossGrams;
            display.ssd1306_command(SSD1306_DISPLAYOFF);
            displayOn = false;
            return;
        }
    }

    // --- Compute net weights ---
    float netBambu     = grossGrams - TARE_BAMBU_G;
    float netCardboard = grossGrams - TARE_CARDBOARD_G;
    float netPlastic   = grossGrams - TARE_PLASTIC_G;

    Serial.printf("Gross: %.1f g  | Bambu: %.1f  Card: %.1f  Plastic: %.1f\n",
                  grossGrams, netBambu, netCardboard, netPlastic);

    // --- Draw display ---
    display.clearDisplay();

    // Yellow zone: "Actual" label + gross weight
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Actual");

    char grossBuf[16];
    snprintf(grossBuf, sizeof(grossBuf), "%5.0fg", grossGrams < 0.0f ? 0.0f : grossGrams);
    int grossW = strlen(grossBuf) * 6;
    display.setCursor(OLED_WIDTH - grossW, 0);
    display.print(grossBuf);

    display.drawFastHLine(0, 14, OLED_WIDTH, SSD1306_WHITE);

    // Blue zone: three net-weight rows, 16px apart (matches size-2 height)
    drawRow(16, "Bambu",   netBambu     < 0.0f ? -1.0f : netBambu);
    drawRow(32, "Cardbd",  netCardboard < 0.0f ? -1.0f : netCardboard);
    drawRow(48, "Plastic", netPlastic   < 0.0f ? -1.0f : netPlastic);

    display.display();
}
