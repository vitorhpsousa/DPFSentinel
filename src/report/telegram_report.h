// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#pragma once
#include <Arduino.h>
#include "../logging/session_store.h"

// Daily Telegram report: once per day after TG_SEND_HOUR:TG_SEND_MIN (local
// time), as soon as the board is online and its clock is set, it sends a short
// summary and uploads every session file it hasn't sent yet. If the board is
// off or offline at that time it simply sends at the next opportunity, so a
// missed evening isn't a lost day. Disabled while TG_BOT_TOKEN is empty.
void reportBegin(SessionStore *store, uint32_t bootNumber);
void reportSendNow();                 // ask for a report right away (test / on demand)
String reportStatusJson();
void reportReset();                   // forget what has been sent (used after tests)
// Queue a one-off alert (e.g. regen started/finished). It is sent as soon as the board is online, so an
// alert raised while offline is delivered later rather than lost.
void reportEvent(const char *text);
// Send the periodic live-status ping right away (test / on demand), instead of waiting for the interval.
void reportStatusNow();
