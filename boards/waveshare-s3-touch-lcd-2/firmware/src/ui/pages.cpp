// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#include "pages.h"
#include <math.h>
#include <string.h>
#include <WiFi.h>
#include "../board/board.h"

static const int W = 240, H = 320, M = 6;
static const float GREEN_MAX = 14.0f, AMBER_MAX = 17.0f, DARK_AT = 28.0f;

static Arduino_Canvas *s_cv = nullptr;
static PageId s_page = PAGE_SOOT;
static bool s_regenNow = false;
static bool s_dirty = true;        // page changed / needs full redraw
static uint32_t s_hash = 0;
static uint32_t s_lastDraw = 0, s_lastDots = 0;

static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
static const uint16_t C_BG = 0x0000, C_FG = 0xFFFF;
static uint16_t C_LBL() { return rgb(0xcc, 0xcc, 0xcc); }
static uint16_t C_DIM() { return rgb(0x3a, 0x3f, 0x44); }
static uint16_t C_GRN() { return rgb(0x1f, 0xa5, 0x4c); }
static uint16_t C_AMB() { return rgb(0xe8, 0xa5, 0x48); }
static uint16_t C_RED() { return rgb(0xd0, 0x2c, 0x20); }

static uint16_t sootColor(float g) {
    if (isnan(g)) return C_DIM();
    if (g < GREEN_MAX) return C_GRN();
    if (g < AMBER_MAX) return C_AMB();
    float t = fminf(1.0f, (g - AMBER_MAX) / (DARK_AT - AMBER_MAX));
    return rgb(0xd0 + (0x8b - 0xd0) * t, 0x2c + (0x00 - 0x2c) * t, 0x20 + (0x00 - 0x20) * t);
}

// ---- text helpers ----
static int textW(const char *t, int size) {
    int16_t x1, y1; uint16_t w, h;
    s_cv->setTextSize(size);
    s_cv->getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
    return w;
}
static int fitSize(const char *t, int maxSize, int maxW) {
    int sz = maxSize;
    while (sz > 1 && textW(t, sz) > maxW) sz--;
    return sz;
}
static void putText(const char *t, int x, int y, int size, uint16_t col) {
    s_cv->setTextSize(size); s_cv->setTextColor(col); s_cv->setCursor(x, y); s_cv->print(t);
}
static int centered(const char *t, int cx, int y, int size, uint16_t col) {
    putText(t, cx - textW(t, size) / 2, y, size, col);
    return y + 8 * size;
}
static void rightText(const char *t, int xr, int y, int size, uint16_t col) {
    putText(t, xr - textW(t, size), y, size, col);
}
// Label (size 1, dim) at left, value (fitted) right-aligned on same row; row height 16.
static void row(int y, const char *label, const char *val, uint16_t col = C_FG) {
    putText(label, M, y + 4, 1, C_LBL());
    int avail = W - 2 * M - textW(label, 1) - 8;
    int sz = fitSize(val, 2, avail);
    rightText(val, W - M, y + (sz == 2 ? 0 : 4), sz, col);
}
static void title(const char *t) {
    putText(t, M, M, 2, C_LBL());
    s_cv->drawFastHLine(M, M + 20, W - 2 * M, C_DIM());
}
static void fmtf(char *o, size_t n, float v, int dec, const char *unit) {
    if (isnan(v)) strlcpy(o, "-", n); else snprintf(o, n, "%.*f%s", dec, v, unit);
}
static void fmtDur(char *o, size_t n, uint32_t s) {
    if (s >= 86400) snprintf(o, n, "%lud %luh", (unsigned long)(s / 86400), (unsigned long)(s % 86400 / 3600));
    else snprintf(o, n, "%lu:%02lu:%02lu", (unsigned long)(s / 3600), (unsigned long)(s % 3600 / 60), (unsigned long)(s % 60));
}

// ---- pages ----
static void drawTrend(const PagesData &d) {
    title("SOOT TREND");
    float cur = NAN, mx = 0;
    for (int i = 239; i >= 0; i--) if (!isnan(d.sootHistory[i])) { cur = d.sootHistory[i]; break; }
    for (int i = 0; i < 240; i++) if (!isnan(d.sootHistory[i]) && d.sootHistory[i] > mx) mx = d.sootHistory[i];
    char b[24];
    fmtf(b, sizeof b, cur, 1, "g");
    int y = centered(b, W / 2, 30, fitSize(b, 5, W - 2 * M), sootColor(cur)) + 6;
    // plot area
    const int px0 = 32, px1 = W - M - 2, py0 = y + 4, py1 = H - 44;
    float ymax = 20.0f;
    while (ymax < mx + 1 && ymax < 60) ymax += 5;
    auto Y = [&](float v) { return py1 - (int)((py1 - py0) * fminf(fmaxf(v, 0), ymax) / ymax + 0.5f); };
    s_cv->drawRect(px0, py0, px1 - px0 + 1, py1 - py0 + 1, C_DIM());
    // y ticks
    for (float t = 0; t <= ymax + 0.01f; t += 10) {
        snprintf(b, sizeof b, "%d", (int)t);
        int ty = Y(t);
        putText(b, px0 - 4 - textW(b, 1), ty - 4, 1, C_LBL());
        if (t > 0 && t < ymax) for (int x = px0 + 1; x < px1; x += 6) s_cv->drawPixel(x, ty, C_DIM());
    }
    // threshold lines
    s_cv->drawFastHLine(px0 + 1, Y(GREEN_MAX), px1 - px0 - 1, C_AMB());
    s_cv->drawFastHLine(px0 + 1, Y(AMBER_MAX), px1 - px0 - 1, C_RED());
    snprintf(b, sizeof b, "14"); putText(b, px1 - 1 - textW(b, 1), Y(GREEN_MAX) + 2, 1, C_AMB());
    snprintf(b, sizeof b, "17"); putText(b, px1 - 1 - textW(b, 1), Y(AMBER_MAX) - 9, 1, C_RED());
    // data: 240 samples over (px1-px0-2) px
    const int pw = px1 - px0 - 2;
    int lx = -1, ly = 0; float lv = NAN;
    for (int i = 0; i < 240; i++) {
        float v = d.sootHistory[i];
        if (isnan(v)) { lx = -1; continue; }
        int x = px0 + 1 + i * (pw - 1) / 239, yy = Y(v);
        if (lx >= 0) {
            uint16_t c = sootColor((v + lv) * 0.5f);
            s_cv->drawLine(lx, ly, x, yy, c);
            s_cv->drawLine(lx, ly + 1, x, yy + 1, c);
        }
        lx = x; ly = yy; lv = v;
    }
    // x labels
    float spanH = d.sootHistoryStepMin * 240 / 60.0f;
    snprintf(b, sizeof b, "-%.0fh", spanH);
    putText(b, px0, py1 + 5, 1, C_LBL());
    snprintf(b, sizeof b, "-%.1fh", spanH / 2);
    putText(b, (px0 + px1) / 2 - textW(b, 1) / 2, py1 + 5, 1, C_LBL());
    rightText("now", px1, py1 + 5, 1, C_LBL());
    putText("grams", M, py1 + 17, 1, C_LBL());
    snprintf(b, sizeof b, "%u min/sample", d.sootHistoryStepMin);
    rightText(b, W - M, py1 + 17, 1, C_LBL());
}

static void drawTrip(const PagesData &d) {
    title("TODAY");
    char b[32];
    int y = 34;
    fmtf(b, sizeof b, d.tripMiles, 1, " mi");
    y = centered(b, W / 2, y, fitSize(b, 5, W - 2 * M), C_FG) + 2;
    centered("DRIVEN", W / 2, y, 1, C_LBL()); y += 18;
    fmtf(b, sizeof b, d.tripSootStart, 1, "g"); row(y, "Soot start", b); y += 22;
    fmtf(b, sizeof b, d.tripSootPeak, 1, "g"); row(y, "Soot peak", b, sootColor(d.tripSootPeak)); y += 22;
    fmtf(b, sizeof b, d.maxSpeed, 0, " km/h"); row(y, "Max speed", b); y += 26;
    s_cv->drawFastHLine(M, y - 4, W - 2 * M, C_DIM());
    snprintf(b, sizeof b, "%u", d.regenCount); row(y, "Regens", b); y += 22;
    if (d.lastRegenEndMs == 0) {
        row(y, "Last regen", "none"); y += 22;
    } else {
        uint32_t ago = (millis() - d.lastRegenEndMs) / 1000;
        if (ago < 3600) snprintf(b, sizeof b, "%lum ago", (unsigned long)(ago / 60));
        else snprintf(b, sizeof b, "%luh%02lum ago", (unsigned long)(ago / 3600), (unsigned long)(ago % 3600 / 60));
        row(y, "Last regen", b); y += 22;
        snprintf(b, sizeof b, "%u min", d.lastRegenMinutes); row(y, "Duration", b); y += 22;
        fmtf(b, sizeof b, d.lastRegenPeakEgt, 0, "\xF8" "C"); row(y, "Peak EGT", b); y += 22;
    }
}

static void drawImu(const PagesData &d) {
    title("MOTION");
    const int cx = W / 2, cy = 112, R = 76;
    s_cv->drawCircle(cx, cy, R, C_LBL());
    s_cv->drawCircle(cx, cy, R / 2, C_DIM());
    s_cv->drawFastHLine(cx - R, cy, 2 * R, C_DIM());
    s_cv->drawFastVLine(cx, cy - R, 2 * R, C_DIM());
    putText("1g", cx + R / 2 + 2, cy + 2, 1, C_DIM());
    const float FS = 1.0f;  // full scale (outer ring) in g
    float lg = isnan(d.longG) ? 0 : d.longG, tg = isnan(d.latG) ? 0 : d.latG;
    float bx = tg / FS * R, by = -lg / FS * R;  // accel = up, right turn = right
    float m = sqrtf(bx * bx + by * by), lim = R - 9;
    if (m > lim) { bx *= lim / m; by *= lim / m; }
    float mag = sqrtf(lg * lg + tg * tg);
    uint16_t bc = mag < 0.3f ? C_GRN() : mag < 0.5f ? C_AMB() : C_RED();
    s_cv->fillCircle(cx + (int)bx, cy + (int)by, 8, bc);
    s_cv->drawCircle(cx + (int)bx, cy + (int)by, 8, C_FG);
    char b[24];
    centered("ACCEL", cx, cy - R - 10, 1, C_LBL());
    centered("BRAKE", cx, cy + R + 3, 1, C_LBL());
    putText("L", cx - R - 8, cy - 4, 1, C_LBL());
    putText("R", cx + R + 3, cy - 4, 1, C_LBL());
    int y = cy + R + 16;
    snprintf(b, sizeof b, "%+.2f", lg); char c[24]; snprintf(c, sizeof c, "%+.2f", tg);
    putText("Long", M, y + 4, 1, C_LBL()); putText(b, M + 30, y, 2, C_FG);
    rightText(c, W - M, y, 2, C_FG); putText("Lat", W - M - textW(c, 2) - 22, y + 4, 1, C_LBL());
    y += 22;
    fmtf(b, sizeof b, d.pitchDeg, 1, "\xF8"); fmtf(c, sizeof c, d.rollDeg, 1, "\xF8");
    putText("Pitch", M, y + 4, 1, C_LBL()); putText(b, M + 36, y, 2, C_FG);
    rightText(c, W - M, y, 2, C_FG); putText("Roll", W - M - textW(c, 2) - 26, y + 4, 1, C_LBL());
    y += 24;
    s_cv->drawFastHLine(M, y - 3, W - 2 * M, C_DIM());
    snprintf(b, sizeof b, "%lu", (unsigned long)d.harshBrake); row(y, "Harsh brake", b); y += 20;
    snprintf(b, sizeof b, "%lu", (unsigned long)d.harshAccel); row(y, "Harsh accel", b); y += 20;
    snprintf(b, sizeof b, "%lu", (unsigned long)d.harshCorner); row(y, "Harsh corner", b);
}

static void drawSystem(const PagesData &d) {
    title("SYSTEM");
    char b[40];
    int y = 30;
    bool wc = WiFi.status() == WL_CONNECTED;
    row(y, "WiFi", wc ? WiFi.SSID().c_str() : "offline"); y += 20;
    row(y, "IP", wc ? WiFi.localIP().toString().c_str() : "-"); y += 20;
    row(y, "BLE adapter", d.bleLinked ? "linked" : "no link", d.bleLinked ? C_GRN() : C_RED()); y += 20;
    snprintf(b, sizeof b, "%s %luMB", d.sdBackend[0] ? d.sdBackend : "none", (unsigned long)d.sdFreeMB);
    row(y, "SD free", b); y += 20;
    snprintf(b, sizeof b, "#%lu", (unsigned long)d.bootNumber); row(y, "Session", b); y += 20;
    fmtf(b, sizeof b, d.batteryV, 2, " V"); row(y, "Battery", b); y += 20;
    row(y, "Clock", d.clockText[0] ? d.clockText : "-", d.clockSynced ? C_FG : C_AMB()); y += 20;
    row(y, "Time sync", d.clockSynced ? "synced" : "NOT synced", d.clockSynced ? C_GRN() : C_AMB()); y += 20;
    fmtDur(b, sizeof b, d.uptimeS); row(y, "Uptime", b); y += 20;
    snprintf(b, sizeof b, "%lu KB", (unsigned long)d.freeHeapKB); row(y, "Free heap", b); y += 20;
    snprintf(b, sizeof b, "%lu KB", (unsigned long)d.freePsramKB); row(y, "Free PSRAM", b);
}

static void drawDots(Arduino_GFX *g, int y) {
    const int gap = 14, x0 = W / 2 - (PAGE_COUNT - 1) * gap / 2;
    g->fillRect(x0 - 8, y - 4, (PAGE_COUNT - 1) * gap + 16, 9, C_BG);
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (i == (int)s_page) g->fillCircle(x0 + i * gap, y, 3, C_FG);
        else g->fillCircle(x0 + i * gap, y, 2, rgb(0x70, 0x76, 0x7c));
    }
}

// ---- public ----
void pagesBegin() {
    Arduino_GFX *g = boardGfx();
    if (!g || s_cv) return;
    s_cv = new Arduino_Canvas(W, H, g);
    if (!s_cv->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("pages: canvas alloc failed");
        delete s_cv; s_cv = nullptr;
    }
    s_dirty = true;
}

void pagesGoto(PageId p) {
    if (s_regenNow || p >= PAGE_COUNT || p == s_page) return;
    s_page = p; s_dirty = true;
}
void pagesNext() { pagesGoto((PageId)((s_page + 1) % PAGE_COUNT)); }
void pagesPrev() { pagesGoto((PageId)((s_page + PAGE_COUNT - 1) % PAGE_COUNT)); }
PageId pagesCurrent() { return s_regenNow ? PAGE_SOOT : s_page; }

void pagesRender(const UiState &ui, const PagesData &d) {
    Arduino_GFX *g = boardGfx();
    if (!g) return;
    bool wasRegen = s_regenNow;
    s_regenNow = ui.hasData && !ui.stale && ui.regen;
    if (s_regenNow) { s_page = PAGE_SOOT; }
    if (wasRegen && !s_regenNow) s_dirty = true;

    if (s_page == PAGE_SOOT) {
        uiRender(ui);
        if (s_regenNow) return;  // regen screen is full screen, no dots
        uint32_t now = millis();
        if (s_dirty || now - s_lastDots > 300) {
            drawDots(g, H - 4);
            s_lastDots = now;
        }
        s_dirty = false;
        return;
    }
    if (!s_cv) return;
    uint32_t now = millis();
    if (!s_dirty && now - s_lastDraw < 500) return;
    s_lastDraw = now;
    s_cv->fillScreen(C_BG);
    switch (s_page) {
        case PAGE_TREND: drawTrend(d); break;
        case PAGE_TRIP: drawTrip(d); break;
        case PAGE_IMU: drawImu(d); break;
        default: drawSystem(d); break;
    }
    drawDots(s_cv, H - 5);
    // hash the framebuffer; flush only if changed
    const uint32_t *p = (const uint32_t *)s_cv->getFramebuffer();
    uint32_t h = 2166136261u;
    for (size_t i = 0, n = (size_t)W * H / 2; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    if (s_dirty || h != s_hash) { s_cv->flush(); s_hash = h; }
    uiInvalidate();   // the panel now shows this page: the soot screen must repaint when it returns
    s_dirty = false;
}

void pagesDemo() {
    static PagesData d;
    static UiState ui;
    static bool init = false;
    if (!init) {
        init = true;
        memset(&d, 0, sizeof d);
        d.sootHistoryStepMin = 2;
        for (int i = 0; i < 240; i++) {
            float v = 6 + i * 0.075f + 1.2f * sinf(i * 0.2f);
            if (i > 150 && i < 156) v = NAN;
            d.sootHistory[i] = v;
        }
        d.tripMiles = 47.3f; d.tripSootStart = 8.1f; d.tripSootPeak = 21.4f; d.regenCount = 1;
        d.lastRegenEndMs = 1; d.lastRegenPeakEgt = 612; d.lastRegenMinutes = 14; d.maxSpeed = 118;
        d.bleLinked = true; strlcpy(d.sdBackend, "SD", sizeof d.sdBackend); d.sdFreeMB = 14820;
        d.bootNumber = 37; d.batteryV = 13.87f; strlcpy(d.clockText, "2026-09-25 14:32", sizeof d.clockText);
        d.clockSynced = true;
        memset(&ui, 0, sizeof ui);
        ui.soot = 19.4f; ui.rpm = 1650; ui.speed = 55; ui.coolant = 90; ui.diffP = 22; ui.catTemp = 340;
        ui.intercooler = 48; ui.maf = 21.5f; ui.sinceRegen = 212; ui.odometer = 124832; ui.hasData = true;
        strlcpy(ui.ip, "192.168.1.42", sizeof ui.ip); strlcpy(ui.ssid, "HomeWiFi", sizeof ui.ssid);
    }
    uint32_t t = millis();
    float a = t / 1000.0f;
    d.longG = 0.45f * sinf(a * 0.9f); d.latG = 0.4f * cosf(a * 0.6f);
    d.pitchDeg = 2.0f * sinf(a * 0.3f); d.rollDeg = 1.5f * cosf(a * 0.4f);
    d.harshBrake = 2; d.harshAccel = 1; d.harshCorner = 4;
    d.uptimeS = t / 1000; d.freeHeapKB = 180; d.freePsramKB = 7600;
    d.lastRegenEndMs = t > 60000 ? t - 3720000UL : 1;
    pagesGoto((PageId)((t / 5000) % PAGE_COUNT));
    pagesRender(ui, d);
}
