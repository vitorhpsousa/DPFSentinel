// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
// Boot self-test: plays each alert quietly. Assumes nothing else uses Wire; we begin it here at 400 kHz
// (in the real app Wire is begun once by the touch code, same pins/speed).
#include <Arduino.h>
#include <Wire.h>
#include "audio/alert_audio.h"

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("audio_test: boot");
  Wire.begin(38, 39, 400000);
  Serial.printf("PA polarity: active %s (GPIO%d)\n", AUDIO_PA_ACTIVE_HIGH ? "HIGH" : "LOW", AUDIO_PIN_PA);
  bool ok = audioBegin();
  Serial.printf("audioBegin: %s\n", ok ? "OK" : "FAILED (codec no ACK or I2S error)");
  if (!ok) return;
  // LOUDNESS DIAGNOSTIC (temporary): four 2-second tones, each preceded by a beep count you can hear.
  auto dacVol = [](uint8_t v) { Wire.beginTransmission(AUDIO_ES8311_ADDR); Wire.write(0x32); Wire.write(v); Wire.endTransmission(); };
  const struct { uint16_t hz; uint8_t vol; const char *n; } t[] = {
    {1000, 0xBF, "1: 1 kHz, DAC 0xBF (as before, 100% cap)"},
    {2500, 0xBF, "2: 2.5 kHz, DAC 0xBF"},
    {1000, 0xFF, "3: 1 kHz, DAC 0xFF (max codec gain)"},
    {2500, 0xFF, "4: 2.5 kHz, DAC 0xFF"}};
  for (int i = 0; i < 4; i++) {
    Serial.printf("tone %s\n", t[i].n);
    dacVol(t[i].vol);
    for (int k = 0; k <= i; k++) { audioBeep(0, 200, 0); }   // silent spacer
    audioBeep(t[i].hz, 2000, 100);
    audioBeep(0, 1500, 0);
    delay(4200 + 200 * (i + 1));
  }
  Serial.println("diagnostic done: which tone (1-4) was loudest?");
}

void loop() { delay(1000); }
