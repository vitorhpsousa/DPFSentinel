// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#include "soot_ui.h"
#include <math.h>
#include <string.h>
#include "../board/board.h"

static const int W = 240, H = 320, MARGIN = 6;
static const float GREEN_MAX = 14.0f, AMBER_MAX = 17.0f, DARK_AT = 28.0f;

static Arduino_Canvas *s_canvas = nullptr;

static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }

static uint16_t sootColor(float g) {
    if (isnan(g)) return rgb(0x3a, 0x3f, 0x44);
    if (g < GREEN_MAX) return rgb(0x1f, 0xa5, 0x4c);
    if (g < AMBER_MAX) return rgb(0xe8, 0xa5, 0x48);
    float t = fminf(1.0f, (g - AMBER_MAX) / (DARK_AT - AMBER_MAX));
    return rgb(0xd0 + (0x8b - 0xd0) * t, 0x2c + (0x00 - 0x2c) * t, 0x20 + (0x00 - 0x20) * t);
}

enum Mode : uint8_t { M_NOLOGGER, M_NOLIVE, M_NORMAL, M_REGEN };

// Everything that is drawn, as text; compared byte-wise to skip redundant frames.
struct Frame {
    uint8_t mode;
    uint16_t bg, fg, lbl;
    char soot[12];
    char val[9][16];
    char cat[20];
    char ip[20];
    char ssid[33];
};
static Frame s_last;
static bool s_haveLast = false;
void uiInvalidate() { s_haveLast = false; }

static void fmt(char *out, size_t n, float v, int dec, const char *unit) {
    if (isnan(v)) strlcpy(out, "-", n);
    else snprintf(out, n, "%.*f%s", dec, v, unit);
}

static void buildFrame(Frame &f, const UiState &s) {
    memset(&f, 0, sizeof f);
    strlcpy(f.ip, s.ip[0] ? s.ip : "no network", sizeof f.ip);
    strlcpy(f.ssid, s.ssid, sizeof f.ssid);
    f.fg = 0xFFFF;
    f.lbl = rgb(0xcc, 0xcc, 0xcc);
    if (!s.hasData || s.stale) {
        f.mode = !s.hasData ? M_NOLOGGER : M_NOLIVE;
        f.bg = rgb(0x3a, 0x3f, 0x44);
        return;
    }
    f.mode = s.regen ? M_REGEN : M_NORMAL;
    f.bg = s.regen ? rgb(0x1c, 0x1c, 0x1c) : sootColor(s.soot);
    if (f.bg == rgb(0xe8, 0xa5, 0x48)) { f.fg = 0x0000; f.lbl = rgb(0x33, 0x33, 0x33); }  // amber: dark text
    fmt(f.soot, sizeof f.soot, s.soot, 1, "g");
    // Order matches the Pi grid after Soot.
    fmt(f.val[0], 16, s.rpm, 0, "");
    fmt(f.val[1], 16, s.speed, 0, " km/h");
    fmt(f.val[2], 16, s.coolant, 0, "\xF8" "C");
    fmt(f.val[3], 16, s.diffP, 1, " hPa");
    fmt(f.val[4], 16, s.catTemp, 0, "\xF8" "C");
    fmt(f.val[5], 16, s.intercooler, 0, "\xF8" "C");
    fmt(f.val[6], 16, s.maf, 1, " g/s");
    fmt(f.val[7], 16, s.sinceRegen, 1, " mi");
    strlcpy(f.val[8], s.regen ? "ACTIVE" : "OFF", 16);
    // (odometer is drawn from val index 9 -> stored below)
}

// Odometer needs a 10th slot; keep it separate to keep Frame simple.
static char s_odo[16], s_lastOdo[16];

// ---- drawing helpers (all width measurement via getTextBounds) ----
static int textW(const char *t, int size) {
    int16_t x1, y1; uint16_t w, h;
    s_canvas->setTextSize(size);
    s_canvas->getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
    return w;
}
// Largest size <= maxSize whose width fits maxW.
static int fitSize(const char *t, int maxSize, int maxW) {
    int sz = maxSize;
    while (sz > 1 && textW(t, sz) > maxW) sz--;
    return sz;
}
// Draws centred at cx with top y; returns y below the line.
static int centered(const char *t, int cx, int y, int size, uint16_t col) {
    s_canvas->setTextSize(size);
    s_canvas->setTextColor(col);
    s_canvas->setCursor(cx - textW(t, size) / 2, y);
    s_canvas->print(t);
    return y + 8 * size;
}
static int left(const char *t, int x, int y, int size, uint16_t col) {
    s_canvas->setTextSize(size);
    s_canvas->setTextColor(col);
    s_canvas->setCursor(x, y);
    s_canvas->print(t);
    return y + 8 * size;
}

static void drawNet(const Frame &f, int y, int size, uint16_t col) {
    // IP line, SSID below; each shrunk to fit.
    int s1 = fitSize(f.ip, size, W - 2 * MARGIN);
    y = centered(f.ip, W / 2, y, s1, col) + 4;
    if (f.ssid[0]) {
        char buf[40];
        snprintf(buf, sizeof buf, "WiFi: %s", f.ssid);
        centered(buf, W / 2, y, fitSize(buf, size, W - 2 * MARGIN), col);
    }
}

static void drawFrame(const Frame &f) {
    const int cx = W / 2;
    s_canvas->fillScreen(f.bg);
    if (f.mode == M_NOLOGGER || f.mode == M_NOLIVE) {
        const char *l1 = f.mode == M_NOLOGGER ? "NO LOGGER" : "NO LIVE";
        const char *l2 = f.mode == M_NOLOGGER ? "DATA" : "DATA";
        int sz = min(fitSize(l1, 4, W - 2 * MARGIN), fitSize(l2, 4, W - 2 * MARGIN));
        int y = 90;
        y = centered(l1, cx, y, sz, f.fg) + 8;
        centered(l2, cx, y, sz, f.fg);
        drawNet(f, 220, 2, f.fg);
        return;
    }
    if (f.mode == M_REGEN) {
        const uint16_t amber = rgb(0xff, 0xc0, 0x40);
        int y = MARGIN + 4;
        y = centered("ACTIVE", cx, y, 3, f.fg) + 4;
        y = centered("REGENERATION", cx, y, fitSize("REGENERATION", 3, W - 2 * MARGIN), f.fg) + 14;
        int sz = min(fitSize("DO NOT SWITCH OFF", 2, W - 2 * MARGIN), fitSize("UNTIL IT FINISHES", 2, W - 2 * MARGIN));
        y = centered("DO NOT SWITCH OFF", cx, y, sz, amber) + 6;
        y = centered("UNTIL IT FINISHES", cx, y, sz, amber) + 20;
        y = centered(f.soot, cx, y, fitSize(f.soot, 7, W - 2 * MARGIN), f.fg) + 6;
        y = centered("SOOT LEVEL", cx, y, 2, f.fg) + 16;
        char b[32];
        snprintf(b, sizeof b, "Cat. Temp %s", f.val[4]);
        centered(b, cx, y, fitSize(b, 2, W - 2 * MARGIN), f.fg);
        drawNet(f, H - MARGIN - 20, 1, f.lbl);
        return;
    }
    // Normal detail grid: big soot on top, 2x5 grid, footer with IP + SSID.
    int y = MARGIN;
    y = left("Soot", MARGIN + 2, y, 2, f.lbl) + 2;
    y = centered(f.soot, cx, y, fitSize(f.soot, 7, W - 2 * MARGIN), f.fg) + 8;
    static const char *labels[10] = {"RPM", "Speed", "Coolant", "Diff P.", "Cat. Temp",
                                     "Intercooler", "MAF", "Since Regen", "Regen", "Odometer"};
    const int footerH = 8 + 4 + 8 + MARGIN;
    const int gridTop = y, gridBottom = H - footerH - 6;
    const int rowH = (gridBottom - gridTop) / 5;   // label(8)+gap(2)+value(16) = 26 <= rowH
    const int colW = W / 2;
    for (int i = 0; i < 10; i++) {
        int x = (i % 2) * colW + MARGIN + 2;
        int ry = gridTop + (i / 2) * rowH;
        const char *v = (i == 9) ? s_odo : f.val[i];
        left(labels[i], x, ry, 1, f.lbl);
        int sz = fitSize(v, 2, colW - MARGIN - 4);
        left(v, x, ry + 10 + (sz == 2 ? 0 : 4), sz, f.fg);
    }
    drawNet(f, H - footerH, 1, f.lbl);
}

void uiBegin() {
    Arduino_GFX *g = boardGfx();
    if (!g || s_canvas) return;
    s_canvas = new Arduino_Canvas(W, H, g);
    if (!s_canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {  // panel already begun by boardInit()
        Serial.println("ui: canvas alloc failed");
        delete s_canvas;
        s_canvas = nullptr;
        return;
    }
    s_haveLast = false;
}

void uiRender(const UiState &s) {
    if (!s_canvas) return;
    Frame f;
    buildFrame(f, s);
    char odo[16];
    fmt(odo, sizeof odo, s.odometer, 0, " mi");
    if (s_haveLast && !memcmp(&f, &s_last, sizeof f) && !strcmp(odo, s_lastOdo)) return;
    strlcpy(s_odo, odo, sizeof s_odo);
    drawFrame(f);
    s_canvas->flush();
    s_last = f;
    strlcpy(s_lastOdo, odo, sizeof s_lastOdo);
    s_haveLast = true;
}

static UiState demoState(float soot, bool regen, float diffP, float since) {
    UiState s;
    s.soot = soot; s.rpm = 1450; s.speed = 42; s.coolant = 88; s.diffP = diffP;
    s.catTemp = 210; s.intercooler = 61; s.maf = 24.3f; s.sinceRegen = since; s.odometer = 124832;
    s.regen = regen; s.hasData = true; s.stale = false;
    strlcpy(s.ip, "192.168.1.42", sizeof s.ip);
    strlcpy(s.ssid, "HomeWiFi", sizeof s.ssid);
    return s;
}

void uiDemo() {
    const uint32_t STEP_MS = 7000;
    static int idx = -1;
    static UiState st;
    int n = (millis() / STEP_MS) % 7;
    if (n == idx) return;
    idx = n;
    switch (n) {
        case 0: st = demoState(NAN, false, 12.5f, 187.4f); st.hasData = false; break;
        case 1: st = demoState(NAN, false, 12.5f, 187.4f); st.stale = true; break;
        case 2: st = demoState(8.2f, false, 12.5f, 187.4f); break;
        case 3: st = demoState(15.5f, false, 18.0f, 187.4f); break;
        case 4: st = demoState(21.0f, false, 26.5f, 312.0f); break;
        case 5: st = demoState(28.0f, false, 34.0f, 455.0f); break;
        default: st = demoState(30.2f, true, 12.5f, 187.4f); st.rpm = 1900; st.speed = 0; break;
    }
    uiRender(st);
}
