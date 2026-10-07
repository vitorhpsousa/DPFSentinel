// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#pragma once
#include <Arduino.h>
#include "../logging/session_store.h"

// Wall-clock time without an RTC. The clock is set at most once per boot, from
// the internet (when the home Wi-Fi is joined) or from the phone that opens
// the dashboard, and it is best-effort: with neither, rows simply carry no
// wall-clock time and nothing else changes. Each time it is set, a TIME marker
// (millis, source, unix time) goes into the raw log, so every earlier row of
// that boot can be dated afterwards (tools/date_session.py).
bool timeValid();
double unixNow();                       // NAN until the clock has been set
const char *timeSource();               // "", "ntp" or "phone"
bool timeSetFromPhone(uint64_t unixMs); // only if not already valid
void timeInitTz();                      // call once at boot: sets the TZ_RULE (DST-aware) for localtime
void timeStartNtp();                    // non-blocking; runs in the background
void timePoll(SessionStore &store);     // call once a second: emits the marker
