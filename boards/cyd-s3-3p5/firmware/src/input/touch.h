// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#pragma once
#include <Arduino.h>

struct TouchEvent {
  enum Type { NONE, TAP, LONG } type;
  int16_t x, y;  // touch-down point (canvas coordinates, 320x480 portrait)
};

bool touchBegin();               // board touch is set up by boardInit(); this only resets the state machine
TouchEvent touchPoll();          // non-blocking; call from the UI task
uint32_t touchLastActivityMs();  // millis() of last touch down/move/up
