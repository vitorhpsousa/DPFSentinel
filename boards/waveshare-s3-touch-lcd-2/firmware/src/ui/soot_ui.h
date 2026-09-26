#pragma once
// Soot panel UI for the 240x320 portrait ST7789 (port of pi/screen/soot_panel.py).
// Draws into a PSRAM canvas and flushes to the panel only when the rendered
// content changed. Built-in Arduino_GFX font only. Call boardInit() first.
#include <Arduino.h>

struct UiState {
    float soot, rpm, speed, coolant, diffP, catTemp, intercooler, maf, sinceRegen, odometer;  // NaN = missing
    bool regen;
    bool hasData;  // false -> "NO LOGGER DATA"
    bool stale;    // true (with hasData) -> "NO LIVE DATA"
    char ip[20];
    char ssid[33];
};

void uiBegin();                       // allocates the canvas (needs boardGfx())
void uiRender(const UiState &s);      // cheap if nothing visible changed
void uiInvalidate();                  // forget the last frame: next uiRender repaints (call after another page drew on the panel)
void uiDemo();                        // cycles 7 demo states, 7 s each; call from loop()
