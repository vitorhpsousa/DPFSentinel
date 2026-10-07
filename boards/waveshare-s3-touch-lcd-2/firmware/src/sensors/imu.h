// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
// QMI8658 IMU (I2C 0x6B, SDA 48 / SCL 47): filtered accel/gyro, car-frame
// longitudinal/lateral G, tilt, motion detection and harsh-driving events.
#pragma once
#include <Arduino.h>

struct ImuSample {
  float ax, ay, az;      // g, board frame, raw
  float gx, gy, gz;      // deg/s, board frame, raw
  float tempC;
  uint32_t ms;           // millis() at read
};

struct ImuStats {
  float longG, latG;         // car frame, gravity removed. +longG = accelerating, -longG = braking;
                             // +latG = acceleration toward the RIGHT (i.e. during a left turn)
  float pitchDeg, rollDeg;   // tilt vs calibrated level (+pitch = nose up, +roll = right side down)
  float peakG10s;            // peak horizontal g (sqrt(long^2+lat^2)) over the last 10 s
  bool moving;
  uint32_t harshBrakeCount, harshAccelCount, harshCornerCount;
  uint32_t lastEventMs;      // millis() of the latest harsh event, 0 = none
};

bool imuBegin();                 // Wire.begin(48,47), configure chip, load calibration. false if no chip.
void imuPoll();                  // non-blocking; call every loop, samples at most every 10 ms
const ImuSample& imuLatest();
const ImuStats& imuStats();
void imuCalibrateLevel();        // car parked level and still: store gravity vector (NVS ns "imu")
void imuCalibrateForward();      // call right after a hard ACCELERATION pulse from standstill
