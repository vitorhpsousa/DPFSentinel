#pragma once
// Touch page manager: soot / trend / trip / IMU / system pages on the 240x320 panel.
// Call uiBegin() and pagesBegin() after boardInit(). Touch handling lives elsewhere:
// call pagesNext()/pagesPrev()/pagesGoto() from it. Built-in Arduino_GFX font only.
#include <Arduino.h>
#include "soot_ui.h"

struct PagesData {
    float sootHistory[240];        // newest last; NaN = none
    uint16_t sootHistoryStepMin;   // minutes per sample
    float tripMiles, tripSootStart, tripSootPeak;
    uint8_t regenCount;
    uint32_t lastRegenEndMs;       // millis() at end of last regen; 0 = none
    float lastRegenPeakEgt;
    uint16_t lastRegenMinutes;
    float maxSpeed;                // km/h
    float longG, latG, pitchDeg, rollDeg;
    uint32_t harshBrake, harshAccel, harshCorner;
    bool bleLinked;
    char sdBackend[8];
    uint32_t sdFreeMB;
    uint32_t bootNumber;
    float batteryV;
    char clockText[24];
    bool clockSynced;
    uint32_t uptimeS;
    uint32_t freeHeapKB, freePsramKB;
};

enum PageId { PAGE_SOOT, PAGE_TREND, PAGE_TRIP, PAGE_IMU, PAGE_SYSTEM, PAGE_COUNT };

void pagesBegin();
void pagesNext();
void pagesPrev();
void pagesGoto(PageId p);
PageId pagesCurrent();
// While ui.regen is true the soot/regen screen is always shown and page changes are ignored.
void pagesRender(const UiState &ui, const PagesData &d);
void pagesDemo();  // fake data, cycles pages every 5 s; call from loop()
