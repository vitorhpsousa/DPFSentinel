// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#include <Arduino.h>
#include <Preferences.h>
#include <string.h>
#include "config.h"
#include "dpf/dpf_monitor.h"
#include "logging/session_store.h"
#include "obd/elm_client.h"
#include "obd/isotp.h"
#include "obd/pid_registry.h"
#include "report/regen_watch.h"
#include "report/telegram_report.h"
#include "web/timekeeper.h"
#include "web/web_ui.h"

static ElmClient elm;
static RegenWatch regenWatch;
static SessionStore store;

#if BLE_SCAN_ONLY

void setup() { Serial.begin(115200); delay(1000); Serial.println("*** BLE_SCAN_ONLY ***"); }
void loop() { ElmClient::scanAndPrint(8000); delay(1500); }

#elif BLE_DUMP_SERVICES

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("*** BLE_DUMP_SERVICES ***");
    if (!elm.begin()) { Serial.println("Connect failed."); return; }
    elm.dumpServices();
}
void loop() { delay(5000); }

#else

// One entry per unique (header, request): several PID_TABLE rows share a
// request (everything in the 2103 reply), so it goes out once and every
// row decodes from the same payload.
struct Req {
    const char *header, *request, *frames, *rxId;
    uint16_t periodMs;
    uint32_t nextDue = 0, lastGood = 0;
    uint8_t fails = 0;
    bool noCount = false;   // adapter rejected the frame-count digit
    uint8_t payload[96];
    size_t len = 0;
};
static Req reqs[PID_TABLE_LEN];
static size_t reqCount = 0;
static int8_t reqOf[PID_TABLE_LEN];

static float values[COLUMN_COUNT];
static int colRpm, colSpeed, colLoad, colDp, colSoot, colEgt, colRegen, colCat, colMap, colBaro, colBoost, colOdo, colDist, colOdoRegen;
static uint32_t bootNumber, lastRow = 0, blankRows = 0;

static int colOf(const char *name) {
    for (size_t i = 0; i < COLUMN_COUNT; i++) if (strcmp(columnName(i), name) == 0) return (int)i;
    return -1;
}

static void buildRequestTable() {
    for (size_t i = 0; i < PID_TABLE_LEN; i++) {
        const PidDef &p = PID_TABLE[i];
        int found = -1;
        for (size_t r = 0; r < reqCount; r++)
            if (!strcmp(reqs[r].header, p.header) && !strcmp(reqs[r].request, p.requestHex)) { found = (int)r; break; }
        if (found < 0) {
            found = (int)reqCount++;
            reqs[found].header = p.header;
            reqs[found].request = p.requestHex;
            reqs[found].frames = p.frames;
            reqs[found].rxId = rxIdFor(p.header);
            reqs[found].periodMs = p.periodMs;
        } else if (p.periodMs < reqs[found].periodMs) {
            reqs[found].periodMs = p.periodMs;
        }
        reqOf[i] = (int8_t)found;
    }
}

static void runRequest(Req &r) {
    uint32_t now = millis();
    bool ok = false;
    if (elm.setHeader(r.header)) {
        const char *frames = r.noCount ? "" : r.frames;
        String raw = elm.queryRaw(r.request, frames);
        // Some clones answer "?" to the ELM327 frame-count digit; retry
        // without it and remember, so this request works on any adapter.
        if (!r.noCount && r.frames[0] && raw.indexOf('?') >= 0 && raw.indexOf('|') < 0) {
            r.noCount = true;
            Serial.printf("Adapter rejected count digit on %s — sending it plain from now on\n", r.request);
            frames = "";
            raw = elm.queryRaw(r.request, frames);
        }
        // While nothing decodes (ECU asleep, adapter still connected) keep only the
        // first few cycles and then one in sixty, so a parked car doesn't fill the
        // storage with "NO DATA".
        static uint32_t deadRawSkip = 0;
        if (blankRows < 5 || (++deadRawSkip % 60) == 0)
            store.queueRaw(now, r.header, (String(r.request) + frames).c_str(), raw);
#if SERIAL_RAW_ECHO
        Serial.printf("RAW %s %s%s -> %s\n", r.header, r.request, frames, raw.c_str());
#endif
        size_t n = isotpExtract(raw.c_str(), r.rxId, r.payload, sizeof(r.payload));
        // The payload must begin with the expected service/PID bytes.
        const PidDef *first = nullptr;
        for (size_t i = 0; i < PID_TABLE_LEN; i++) if (&reqs[reqOf[i]] == &r) { first = &PID_TABLE[i]; break; }
        if (n > 0 && first && n >= first->prefixLen && !memcmp(r.payload, first->prefix, first->prefixLen)) {
            r.len = n;
            r.lastGood = now;
            r.fails = 0;
            ok = true;
        }
    }
    if (!ok) { if (r.fails < 255) r.fails++; }
    // Requests the ECU never answers (the standard probes) back off to one
    // try every 30 s instead of stalling the fast loop on ATST96 timeouts.
    uint32_t period = (r.fails >= 5) ? 30000 : r.periodMs;
    r.nextDue = now + period;
}

static void assembleRow() {
    uint32_t now = millis();
    for (size_t i = 0; i < PID_TABLE_LEN; i++) {
        const Req &r = reqs[reqOf[i]];
        uint32_t stale = r.periodMs * 3 > 3000 ? r.periodMs * 3 : 3000;
        values[i] = (r.lastGood && now - r.lastGood <= stale) ? PID_TABLE[i].decode(r.payload, r.len) : NAN;
    }
    float odo = values[colOdo], dist = values[colDist];
    values[colOdoRegen] = (!isnan(odo) && !isnan(dist)) ? odo - dist : NAN;
    // Boost (mbar) = enhanced intake MAP (22280B, mbar) - barometric pressure
    // (0133, kPa) x 10. Only exists while the ECU answers both.
    float boost = (!isnan(values[colMap]) && !isnan(values[colBaro])) ? values[colMap] - values[colBaro] * 10.0f : NAN;
#if BOOST_UNIT == 0
    if (!isnan(boost)) boost /= 10.0f;            // kPa
#elif BOOST_UNIT == 1
    if (!isnan(boost)) boost *= 0.0145038f;       // PSI
#endif
    values[colBoost] = boost;
}

static void printRow() {
    Serial.printf("t=%lus rpm=%.0f spd=%.0f load=%.0f%% dpfP=%.1fhPa soot=%.1fg egt=%.0fC cat=%.0fC regen=%.0f since=%.1fmi boost=%.1f | %s\n",
                  (unsigned long)(millis() / 1000), values[colRpm], values[colSpeed], values[colLoad], values[colDp],
                  values[colSoot], values[colEgt], values[colCat], values[colRegen], values[colDist], values[colBoost],
                  store.backend());
}

static bool connectAdapter() {
    elm.close();
    if (!elm.begin()) return false;
    if (!elm.initAdapter()) { Serial.println("Adapter init sequence failed"); return false; }
    Serial.println("ELM327 adapter ready");
    for (size_t r = 0; r < reqCount; r++) { reqs[r].nextDue = 0; reqs[r].fails = 0; }
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    timeInitTz();

    Preferences prefs;
    prefs.begin("obd", false);
    bootNumber = prefs.getUInt("boot", 0) + 1;
    prefs.putUInt("boot", bootNumber);
    prefs.end();

    for (size_t i = 0; i < COLUMN_COUNT; i++) values[i] = NAN;
    colRpm = colOf("engine_rpm"); colSpeed = colOf("vehicle_speed"); colLoad = colOf("engine_load_pct");
    colDp = colOf("dpf_diff_pressure_hpa"); colSoot = colOf("dpf_soot_level_g"); colEgt = colOf("egt_before_dpf_c");
    colRegen = colOf("dpf_regen_active"); colCat = colOf("dpf_zone_temp_c"); colMap = colOf("intake_map_mbar");
    colBaro = colOf("baro_kpa"); colBoost = colOf("turbo_boost"); colOdo = colOf("odometer_mi");
    colDist = colOf("dpf_dist_since_regen_mi"); colOdoRegen = colOf("dpf_odo_at_last_regen_mi");
    buildRequestTable();

    if (store.begin(bootNumber)) Serial.printf("Logging to %s as session_%lu.csv (+ raw_%lu.log)\n", store.backend(),
                                               (unsigned long)bootNumber, (unsigned long)bootNumber);
    else Serial.println("Storage init failed — logging to Serial only");

    pinMode(LED_REGEN_PIN, OUTPUT);
    pinMode(LED_TEMP_WARN_PIN, OUTPUT);
    digitalWrite(LED_REGEN_PIN, LOW);
    digitalWrite(LED_TEMP_WARN_PIN, LOW);

#if WEB_ENABLED
    webUiBegin(&store, bootNumber);
    reportBegin(&store, bootNumber);
#endif

    Serial.println("Connecting to BLE ELM327 adapter...");
    connectAdapter();
}

static void serialCommands() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == 'd') { Serial.println("--- dumping logs ---"); store.dumpAll(Serial); Serial.println("--- dump complete ---"); }
        else if (c == 'p') { store.tailSession(Serial, bootNumber - 1, 240); }
        else if (c == 'e') { store.eraseAll(); Serial.println("--- logs erased, reboot to start a new session ---"); }
    }
}

void loop() {
    static uint32_t lastReconnect = 0;
    serialCommands();
    uint32_t now = millis();

    // Housekeeping that must not depend on the adapter link: clock marker, raw
    // log flush, and the web page's view of the current state.
    static uint32_t lastHousekeeping = 0;
    if (now - lastHousekeeping >= 1000) {
        lastHousekeeping = now;
        timePoll(store);
        store.flushRaw();
#if WEB_ENABLED
        webUiUpdate(values, COLUMN_COUNT, elm.connected());
#endif
    }

    if (!elm.connected()) {
        if (now - lastReconnect >= 5000) {
            lastReconnect = now;
            Serial.println("Adapter link down — reconnecting...");
            for (size_t i = 0; i < COLUMN_COUNT; i++) values[i] = NAN;
            store.writeRow(now, unixNow(), values, COLUMN_COUNT);
            connectAdapter();
        }
        delay(50);
        return;
    }

    // Serve the most overdue request; the loop never blocks longer than one
    // adapter round trip, so fast PIDs (RPM, load) keep their cadence while
    // slow ones (coolant, EGR, EPS) simply come round less often.
    Req *best = nullptr;
    for (size_t r = 0; r < reqCount; r++) {
        if (now >= reqs[r].nextDue && (!best || reqs[r].nextDue < best->nextDue)) best = &reqs[r];
    }
    if (best) runRequest(*best);

    now = millis();
    if (now - lastRow >= ROW_INTERVAL_MS) {
        lastRow = now;
        assembleRow();
        store.writeRow(now, unixNow(), values, COLUMN_COUNT);
        printRow();
#if TG_ALERT_REGEN
        {
            RegenEvent ev = regenWatch.update(now, unixNow(), values[colRegen], values[colEgt], values[colSoot], values[colDist], values[colDp]);
            if (ev.type != RegenEvent::NONE) {
                Serial.printf("REGEN EVENT: %s\n", ev.text);
                reportEvent(ev.text);
            }
        }
#endif

        bool any = false;
        for (size_t i = 0; i < PID_TABLE_LEN; i++) if (!isnan(values[i])) { any = true; break; }
        blankRows = any ? 0 : blankRows + 1;
        if (blankRows == BLANK_ROWS_REINIT) {
            Serial.println("No decodable data — re-initialising adapter");
            elm.initAdapter();
            for (size_t r = 0; r < reqCount; r++) { reqs[r].nextDue = 0; reqs[r].fails = 0; }
        } else if (blankRows >= BLANK_ROWS_RECONNECT) {
            blankRows = 0;
            Serial.println("Still no data — reconnecting BLE");
            elm.close();
        }

        if (isDpfIdleBlockageFlagged(values[colDp], values[colRpm]))
            Serial.println("*** DPF idle pressure elevated — possible blockage ***");
        digitalWrite(LED_REGEN_PIN, (!isnan(values[colRegen]) && values[colRegen] != 0.0f) ? HIGH : LOW);
        float hot = !isnan(values[colEgt]) ? values[colEgt] : values[colCat];
        digitalWrite(LED_TEMP_WARN_PIN, (!isnan(hot) && hot >= DPF_TEMP_WARN_THRESHOLD_C) ? HIGH : LOW);
    }
    delay(1);
}

#endif
