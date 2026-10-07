// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#pragma once
#include <Arduino.h>

struct TouchEvent {
  enum Type { NONE, TAP, LONG_PRESS, SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN } type;
  int16_t x, y;  // TAP/LONG_PRESS: touch point; swipes: start point
};

bool touchBegin();               // safe if Wire already started elsewhere
TouchEvent touchPoll();          // non-blocking; call every loop
uint32_t touchLastActivityMs();  // millis() of last touch down/move/up
