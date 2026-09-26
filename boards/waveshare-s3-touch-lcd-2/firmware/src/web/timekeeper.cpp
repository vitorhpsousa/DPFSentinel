#include "timekeeper.h"
#include "../config.h"
#include <sys/time.h>
#include <time.h>

static char gSrc[8] = "";
static bool gPending = false;
static double gSetUnix = 0;
static uint32_t gSetMs = 0;

bool timeValid() { return time(nullptr) > 1735689600; }  // after 2025-01-01

double unixNow() {
    if (!timeValid()) return NAN;
    timeval tv;
    gettimeofday(&tv, nullptr);
    return (double)tv.tv_sec + tv.tv_usec / 1e6;
}

const char *timeSource() { return gSrc; }

static void noteSet(const char *src) {
    strncpy(gSrc, src, sizeof(gSrc) - 1);
    gSetUnix = unixNow();
    gSetMs = millis();
    gPending = true;
}

bool timeSetFromPhone(uint64_t unixMs) {
    if (timeValid() || unixMs < 1735689600000ULL) return false;
    timeval tv;
    tv.tv_sec = (time_t)(unixMs / 1000ULL);
    tv.tv_usec = (suseconds_t)((unixMs % 1000ULL) * 1000ULL);
    settimeofday(&tv, nullptr);
    noteSet("phone");
    return true;
}

// Applied at boot, independent of NTP: the clock can also be set from the
// phone (no internet), and localtime_r() must still apply the DST rule then.
void timeInitTz() {
    setenv("TZ", TZ_RULE, 1);
    tzset();
}

void timeStartNtp() { configTzTime(TZ_RULE, "pool.ntp.org", "time.google.com"); }

void timePoll(SessionStore &store) {
    if (!gSrc[0] && timeValid()) noteSet("ntp");
    if (gPending) {
        gPending = false;
        store.queueRaw(gSetMs, "TIME", gSrc, String(gSetUnix, 3));
        Serial.printf("Clock set from %s: %.0f (unix)\n", gSrc, gSetUnix);
    }
}
