// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
// Read-only hardware probe: I2C scan, IMU id, touch, SD card, display, PSRAM.
// Results go to Serial (USB CDC, 115200) and onto the screen.
#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Arduino_GFX_Library.h>

#define PIN_SCLK 39
#define PIN_MOSI 38
#define PIN_MISO 40
#define PIN_DC 42
#define PIN_CS 45
#define PIN_SD_CS 41
#define PIN_BL 1
#define PIN_SDA 48
#define PIN_SCL 47
#define PIN_BATT 5
#define TOUCH_ADDR 0x15
#define IMU_ADDR 0x6B

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO, FSPI, true);
Arduino_GFX *gfx = new Arduino_ST7789(bus, -1, 1, true, 240, 320);

String report;
void say(const String &s) { Serial.println(s); report += s + "\n"; }

void probeI2c() {
  Wire.begin(PIN_SDA, PIN_SCL);
  String found;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) found += String("0x") + String(a, HEX) + " ";
  }
  say("I2C: " + (found.length() ? found : String("nothing")));
  Wire.beginTransmission(IMU_ADDR); Wire.write(0x00); Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)1);
  if (Wire.available()) { uint8_t id = Wire.read(); say("IMU WHO_AM_I=0x" + String(id, HEX) + (id == 0x05 ? " (QMI8658 ok)" : " (unexpected)")); }
  else say("IMU: no reply");
}

static void imuWrite(uint8_t r, uint8_t v) { Wire.beginTransmission(IMU_ADDR); Wire.write(r); Wire.write(v); Wire.endTransmission(); }
static bool imuRead(uint8_t r, uint8_t *buf, size_t n) {
  Wire.beginTransmission(IMU_ADDR); Wire.write(r); if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)n) != n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}
// QMI8658 (datasheet register map, unverified against Waveshare code): +-8 g, +-1024 dps, 500 Hz
static void imuStart() {
  imuWrite(0x60, 0xB0); delay(20);   // soft reset
  imuWrite(0x02, 0x40);              // auto-increment register reads
  imuWrite(0x03, 0x25);              // accel: +-8 g, ODR code 5
  imuWrite(0x04, 0x65);              // gyro: +-1024 dps, ODR code 5
  imuWrite(0x08, 0x03);              // enable accel + gyro
  delay(100);
}
static void imuStream(uint32_t ms) {
  float amin = 99, amax = 0, gmax = 0, asum = 0; int n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    uint8_t b[14];
    if (imuRead(0x33, b, 14)) {
      auto s16 = [&](int i) { return (int16_t)(b[i] | (b[i + 1] << 8)); };
      float temp = s16(0) / 256.0f;
      float ax = s16(2) / 4096.0f, ay = s16(4) / 4096.0f, az = s16(6) / 4096.0f;
      float gx = s16(8) / 32.0f, gy = s16(10) / 32.0f, gz = s16(12) / 32.0f;
      float am = sqrtf(ax * ax + ay * ay + az * az), gm = sqrtf(gx * gx + gy * gy + gz * gz);
      amin = min(amin, am); amax = max(amax, am); gmax = max(gmax, gm); asum += am; n++;
      if (n % 25 == 1) Serial.printf("IMU a=(%.2f,%.2f,%.2f)g |a|=%.2f  g=(%.0f,%.0f,%.0f)dps |g|=%.0f  T=%.1fC\n", ax, ay, az, am, gx, gy, gz, gm, temp);
    }
    delay(10);
  }
  say(String("IMU summary: ") + n + " samples, |a| min " + String(amin, 2) + " mean " + String(n ? asum / n : 0, 2) + " max " + String(amax, 2) + " g, peak gyro " + String(gmax, 0) + " dps");
}

void probeSd() {
  pinMode(PIN_CS, OUTPUT); digitalWrite(PIN_CS, HIGH);       // deselect the LCD so it can't drive MISO
  pinMode(PIN_SD_CS, OUTPUT); digitalWrite(PIN_SD_CS, HIGH);
  const uint32_t speeds[] = {400000, 4000000, 10000000, 20000000};
  for (uint32_t hz : speeds) {
    SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI, PIN_SD_CS);
    if (SD.begin(PIN_SD_CS, SPI, hz)) {
      uint8_t t = SD.cardType();
      say(String("SD: mounted at ") + String(hz / 1000) + " kHz, type " + (t == CARD_MMC ? "MMC" : t == CARD_SD ? "SDSC" : t == CARD_SDHC ? "SDHC" : "?") +
          ", " + String((uint32_t)(SD.cardSize() / (1024 * 1024))) + " MB");
      File f = SD.open("/probe.txt", FILE_WRITE);
      if (f) { f.print("probe ok"); f.close(); }
      f = SD.open("/probe.txt");
      String back = f ? f.readString() : "";
      if (f) f.close();
      SD.remove("/probe.txt");
      say(back == "probe ok" ? "SD: write+read OK" : "SD: write/read FAILED");
      SD.end(); SPI.end();
      return;
    }
    say(String("SD: no mount at ") + String(hz / 1000) + " kHz");
    SD.end(); SPI.end();
  }
  say("SD: FAILED at every speed");
}

void setup() {
  Serial.begin(115200);
  delay(2500);
  say("PROBE Waveshare ESP32-S3-Touch-LCD-2");
  say("PSRAM: " + String(ESP.getPsramSize() / 1024) + " KB, free heap " + String(ESP.getFreeHeap() / 1024) + " KB");
  say("Flash: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB");
  probeI2c();
  imuStart();
  Serial.println("IMU: hold still 4 s, then tilt/shake the board for 10 s");
  imuStream(4000);
  say("IMU: now MOVE it");
  imuStream(10000);
  probeSd();
  say("Battery ADC raw=" + String(analogRead(PIN_BATT)) + " (~" + String(3.3f / 4096 * analogRead(PIN_BATT) * 3, 2) + " V if 1:3)");
  pinMode(PIN_BL, OUTPUT); digitalWrite(PIN_BL, HIGH);
  if (!gfx->begin()) { say("Display: begin FAILED"); return; }
  gfx->fillScreen(RED); delay(400); gfx->fillScreen(GREEN); delay(400); gfx->fillScreen(BLUE); delay(400);
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE); gfx->setTextSize(1); gfx->setCursor(2, 2);
  gfx->print(report);
  say("Display: drawn (did you see red, green, blue?)");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last < 200) return;
  last = millis();
  Wire.beginTransmission(TOUCH_ADDR); Wire.write(0x01); Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)6);
  if (Wire.available() >= 6) {
    uint8_t b[6]; for (auto &x : b) x = Wire.read();
    if (b[1] > 0) {
      int x = ((b[2] & 0x0f) << 8) | b[3], y = ((b[4] & 0x0f) << 8) | b[5];
      Serial.printf("touch x=%d y=%d\n", x, y);
      gfx->fillCircle(x, y, 4, YELLOW);
    }
  }
}
