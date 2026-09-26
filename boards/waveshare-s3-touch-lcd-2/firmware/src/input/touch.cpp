#include "touch.h"
#include <Wire.h>
#include "esp32-hal-i2c.h"

static const uint8_t TOUCH_ADDR = 0x15;
static const uint32_t POLL_MS = 20;         // ~50 Hz
static const uint32_t TAP_MAX_MS = 500;
static const uint32_t LONG_MS = 700;
static const uint32_t LOCKOUT_MS = 60;
static const uint32_t UP_DEBOUNCE_MS = 40;  // finger absent this long = released
static const int TAP_MAX_MOVE = 12;
static const int SWIPE_MIN = 25;   // was 40: the panel's reported position lags the finger (see trace 2026-09-25)

static bool ready = false;
static uint32_t lastPoll = 0, lastActivity = 0, lockUntil = 0;
static bool down = false, longFired = false;
static uint32_t downAt = 0, lastSeen = 0;
static int16_t sx, sy, lx, ly;
static int nSamples = 0;   // DEBUG
static uint8_t latchedGesture = 0;   // controller's own gesture id (0x01 up, 0x02 down, 0x03 left, 0x04 right: datasheet, checked in the trace)

bool touchBegin() {
  if (!i2cIsInit(0)) Wire.begin(48, 47);
  Wire.setClock(400000);
  Wire.beginTransmission(TOUCH_ADDR);
  ready = (Wire.endTransmission() == 0);
  if (ready) {   // CST816: disable the controller's auto-standby (reg 0xFE): it can drop or garble touches after idling
    Wire.beginTransmission(TOUCH_ADDR); Wire.write(0xFE); Wire.write(0x01); Wire.endTransmission();
    Wire.beginTransmission(TOUCH_ADDR); Wire.write(0xA7); Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)1);
    Serial.printf("Touch chip id 0x%02x (CST816 = 0xB6)\n", Wire.available() ? Wire.read() : 0);
  }
  lastActivity = millis();
  return ready;
}

uint32_t touchLastActivityMs() { return lastActivity; }

static uint8_t lastGestureByte = 0;
static bool readTouch(int16_t &x, int16_t &y, bool &pressed) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x01);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)6) != 6) return false;
  uint8_t b[6];
  for (auto &v : b) v = Wire.read();
  pressed = b[1] > 0;
  lastGestureByte = b[0];
  x = ((b[2] & 0x0f) << 8) | b[3];
  y = ((b[4] & 0x0f) << 8) | b[5];
  if (pressed && (x > 239 || y > 319)) return false;   // torn/garbled read: ignore
  return true;
}

TouchEvent touchPoll() {
  TouchEvent ev = {TouchEvent::NONE, 0, 0};
  if (!ready) return ev;
  uint32_t now = millis();
  if (now - lastPoll < POLL_MS) return ev;
  lastPoll = now;

  int16_t x = 0, y = 0;
  bool pressed = false;
  if (!readTouch(x, y, pressed)) return ev;

  if (now < lockUntil) {  // swallow input during lockout
    if (pressed) lastActivity = now;
    down = false;
    return ev;
  }

  if (pressed) {
    lastActivity = now;
    lastSeen = now;
    if (lastGestureByte >= 1 && lastGestureByte <= 4) latchedGesture = lastGestureByte;
    if (!down) {
      down = true; longFired = false; downAt = now;
      sx = lx = x; sy = ly = y; nSamples = 1; latchedGesture = 0;
    } else {
      lx = x; ly = y; nSamples++;

      if (!longFired && now - downAt >= LONG_MS &&
          abs(lx - sx) < TAP_MAX_MOVE && abs(ly - sy) < TAP_MAX_MOVE) {
        longFired = true;
        ev = {TouchEvent::LONG_PRESS, sx, sy};
        lockUntil = now + LOCKOUT_MS;
      }
    }
    return ev;
  }

  if (!down || now - lastSeen < UP_DEBOUNCE_MS) return ev;

  // released
  down = false;
  lastActivity = now;
  if (longFired) return ev;
  int dx = lx - sx, dy = ly - sy;
  int adx = abs(dx), ady = abs(dy);
  if (latchedGesture) {   // trust the controller's own gesture recogniser first
    static const TouchEvent::Type map[5] = {TouchEvent::NONE, TouchEvent::SWIPE_UP, TouchEvent::SWIPE_DOWN, TouchEvent::SWIPE_LEFT, TouchEvent::SWIPE_RIGHT};
    ev = {map[latchedGesture], sx, sy};
  } else if (adx < TAP_MAX_MOVE && ady < TAP_MAX_MOVE) {
    if (lastSeen - downAt < TAP_MAX_MS) ev = {TouchEvent::TAP, sx, sy};
  } else if (adx >= SWIPE_MIN || ady >= SWIPE_MIN) {
    if (adx >= ady) ev = {dx < 0 ? TouchEvent::SWIPE_LEFT : TouchEvent::SWIPE_RIGHT, sx, sy};
    else ev = {dy < 0 ? TouchEvent::SWIPE_UP : TouchEvent::SWIPE_DOWN, sx, sy};
  }
  if (ev.type != TouchEvent::NONE) lockUntil = now + LOCKOUT_MS;
  return ev;
}
