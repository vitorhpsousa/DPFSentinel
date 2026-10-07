// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#include "imu.h"
#include <Wire.h>
#include <Preferences.h>
#include <math.h>

static const uint8_t ADDR = 0x6B;
static const int PIN_SDA = 48, PIN_SCL = 47;

// ---- Tunables. ALL thresholds below are UNVERIFIED defaults, not tuned in a car. ----
static const uint32_t POLL_MS        = 10;
static const float    ACC_ALPHA      = 0.10f;   // LPF on accel for event/G values (~100 ms tau at 10 ms)
static const float    TILT_ALPHA     = 0.01f;   // slow LPF for pitch/roll (~1 s tau)
static const float    VIB_ALPHA      = 0.05f;   // EMA of vibration variance
static const float    BRAKE_G        = -0.35f;  // harsh brake: longG below this ...
static const float    ACCEL_G        = 0.30f;   // harsh accel: longG above this ...
static const float    CORNER_G       = 0.40f;   // harsh corner: |latG| above this ...
static const uint32_t SUSTAIN_MS     = 200;     // ... for this long
static const uint32_t REFRACT_MS     = 3000;    // per event type
static const float    MOVE_VIB_VAR   = 0.0003f; // g^2 of raw-vs-filtered accel (std ~0.017 g) => moving
static const float    MOVE_GYRO_DPS  = 3.0f;    // EMA of |gyro| above this => moving (bias/drift is ~1 dps)
static const uint32_t MOVE_HOLD_MS   = 2000;    // condition must hold this long to flip state either way
static const float    FWD_MIN_G      = 0.15f;   // min horizontal pulse to accept a forward calibration

struct V3 { float x, y, z; };
static inline V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static inline float len(V3 a) { return sqrtf(dot(a, a)); }
static inline V3 scale(V3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
static inline V3 unit(V3 a) { float l = len(a); return l > 1e-6f ? scale(a, 1.0f / l) : a; }

static ImuSample s_latest;
static ImuStats  s_stats;
static bool      s_ok = false;
static uint32_t  s_lastPoll = 0;
static bool      s_havePrev = false;

static V3 s_g0  = {0, 0, 1};   // gravity vector at level, board frame (points "up" in accel reading)
static V3 s_fwd = {1, 0, 0};   // forward axis, board frame, unverified default until calibrated
static V3 s_lat;               // derived: unit vector toward car's right, board frame
static V3 s_up;

static V3 s_aLp;               // fast-filtered accel
static V3 s_aTilt;             // slow-filtered accel (gravity estimate)
static float s_vibVar = 0, s_gyroEma = 0;
static uint32_t s_moveSince = 0;  // when candidate state began (0 = none)
static bool s_moveCand = false;

// per-event sustain/refractory
struct Ev { uint32_t since; uint32_t lastFire; };
static Ev s_evBrake, s_evAccel, s_evCorner;

// 1 s buckets for 10 s peak
static float s_peakBucket[10];
static uint32_t s_bucketIdx = 0, s_bucketStart = 0;

// last ~1 s of horizontal linear accel, for forward calibration
static const int HIST = 100;
static V3 s_hist[HIST];
static int s_histPos = 0;

static Preferences s_prefs;

static void deriveAxes() {
  s_up = unit(s_g0);
  V3 f = sub(s_fwd, scale(s_up, dot(s_fwd, s_up)));   // project forward onto horizontal plane
  if (len(f) < 1e-3f) { f = {1, 0, 0}; f = sub(f, scale(s_up, dot(f, s_up))); }
  s_fwd = unit(f);
  s_lat = unit(cross(s_fwd, s_up));                   // fwd x up = right-hand side
}

static void savePrefs() {
  s_prefs.begin("imu", false);
  float a[6] = {s_g0.x, s_g0.y, s_g0.z, s_fwd.x, s_fwd.y, s_fwd.z};
  s_prefs.putBytes("cal", a, sizeof(a));
  s_prefs.end();
}

static void loadPrefs() {
  s_prefs.begin("imu", true);
  float a[6];
  if (s_prefs.getBytesLength("cal") == sizeof(a)) {
    s_prefs.getBytes("cal", a, sizeof(a));
    s_g0 = {a[0], a[1], a[2]}; s_fwd = {a[3], a[4], a[5]};
  }
  s_prefs.end();
}

static void wr(uint8_t r, uint8_t v) { Wire.beginTransmission(ADDR); Wire.write(r); Wire.write(v); Wire.endTransmission(); }
static bool rd(uint8_t r, uint8_t *b, size_t n) {
  Wire.beginTransmission(ADDR); Wire.write(r);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)ADDR, (uint8_t)n) != n) return false;
  for (size_t i = 0; i < n; i++) b[i] = Wire.read();
  return true;
}

bool imuBegin() {
  Wire.begin(PIN_SDA, PIN_SCL);           // harmless if the bus is already up on the same pins
  uint8_t id = 0;
  if (!rd(0x00, &id, 1) || id != 0x05) return false;
  wr(0x60, 0xB0); delay(20);              // soft reset
  wr(0x02, 0x40);                         // auto-increment
  wr(0x03, 0x25);                         // accel +-8 g
  wr(0x04, 0x65);                         // gyro +-1024 dps
  wr(0x08, 0x03);                         // enable accel + gyro
  delay(100);
  memset(&s_stats, 0, sizeof(s_stats));
  memset(&s_latest, 0, sizeof(s_latest));
  memset(s_peakBucket, 0, sizeof(s_peakBucket));
  memset(s_hist, 0, sizeof(s_hist));
  s_evBrake = s_evAccel = s_evCorner = {0, 0};
  s_havePrev = false; s_vibVar = s_gyroEma = 0; s_moveSince = 0; s_moveCand = false;
  loadPrefs();
  deriveAxes();
  s_ok = true;
  return true;
}

const ImuSample& imuLatest() { return s_latest; }
const ImuStats& imuStats() { return s_stats; }

void imuCalibrateLevel() {
  if (!s_ok || !s_havePrev) return;
  s_g0 = s_aTilt;                          // slow-filtered = best gravity estimate
  deriveAxes();
  savePrefs();
}

void imuCalibrateForward() {
  if (!s_ok || !s_havePrev) return;
  // Find the strongest horizontal (relative to level) acceleration in the last ~1 s.
  V3 best = {0, 0, 0}; float bm = 0;
  for (int i = 0; i < HIST; i++) {
    V3 h = s_hist[i];
    h = sub(h, scale(s_up, dot(h, s_up)));
    float m = len(h);
    if (m > bm) { bm = m; best = h; }
  }
  if (bm < FWD_MIN_G) return;              // no clear pulse: keep old axis
  s_fwd = unit(best);                      // assumes the pulse was ACCELERATION (a brake would flip it)
  deriveAxes();
  savePrefs();
}

// Fires when cond holds continuously for SUSTAIN_MS; then locks out for REFRACT_MS.
static bool evCheck(Ev &e, bool cond, uint32_t now) {
  if (!cond) { e.since = 0; return false; }
  if (e.lastFire && now - e.lastFire < REFRACT_MS) { e.since = 0; return false; }
  if (!e.since) { e.since = now; return false; }
  if (now - e.since >= SUSTAIN_MS) { e.since = 0; e.lastFire = now; return true; }
  return false;
}

void imuPoll() {
  if (!s_ok) return;
  uint32_t now = millis();
  if (s_havePrev && now - s_lastPoll < POLL_MS) return;
  s_lastPoll = now;

  uint8_t b[14];
  if (!rd(0x33, b, 14)) return;
  auto s16 = [&](int i) { return (int16_t)(b[i] | (b[i + 1] << 8)); };
  ImuSample &s = s_latest;
  s.tempC = s16(0) / 256.0f;
  s.ax = s16(2) / 4096.0f; s.ay = s16(4) / 4096.0f; s.az = s16(6) / 4096.0f;
  s.gx = s16(8) / 32.0f;   s.gy = s16(10) / 32.0f;  s.gz = s16(12) / 32.0f;
  s.ms = now;

  V3 a = {s.ax, s.ay, s.az};
  if (!s_havePrev) {
    s_aLp = s_aTilt = a; s_havePrev = true; s_bucketStart = now;
  }
  V3 dev = sub(a, s_aLp);                              // raw minus filtered = vibration
  s_aLp = {s_aLp.x + ACC_ALPHA * (a.x - s_aLp.x), s_aLp.y + ACC_ALPHA * (a.y - s_aLp.y), s_aLp.z + ACC_ALPHA * (a.z - s_aLp.z)};
  s_aTilt = {s_aTilt.x + TILT_ALPHA * (a.x - s_aTilt.x), s_aTilt.y + TILT_ALPHA * (a.y - s_aTilt.y), s_aTilt.z + TILT_ALPHA * (a.z - s_aTilt.z)};

  // Car frame, gravity removed (calibrated level gravity subtracted).
  V3 lin = sub(s_aLp, s_g0);
  s_hist[s_histPos] = lin; s_histPos = (s_histPos + 1) % HIST;
  ImuStats &st = s_stats;
  st.longG = dot(lin, s_fwd);
  st.latG  = dot(lin, s_lat);

  // Tilt from slow gravity estimate, in the car frame.
  st.pitchDeg = atan2f(dot(s_aTilt, s_fwd), dot(s_aTilt, s_up)) * 57.29578f;
  st.rollDeg  = atan2f(dot(s_aTilt, s_lat), dot(s_aTilt, s_up)) * 57.29578f;

  // 10 s peak of horizontal g in 1 s buckets.
  while (now - s_bucketStart >= 1000) {
    s_bucketStart += 1000; s_bucketIdx = (s_bucketIdx + 1) % 10; s_peakBucket[s_bucketIdx] = 0;
  }
  float hm = sqrtf(st.longG * st.longG + st.latG * st.latG);
  if (hm > s_peakBucket[s_bucketIdx]) s_peakBucket[s_bucketIdx] = hm;
  float pk = 0; for (int i = 0; i < 10; i++) if (s_peakBucket[i] > pk) pk = s_peakBucket[i];
  st.peakG10s = pk;

  // Moving: vibration variance or sustained gyro activity, held 2 s (hysteresis both ways).
  float dv = dot(dev, dev);
  s_vibVar += VIB_ALPHA * (dv - s_vibVar);
  float gm = sqrtf(s.gx * s.gx + s.gy * s.gy + s.gz * s.gz);
  s_gyroEma += VIB_ALPHA * (gm - s_gyroEma);
  bool active = s_vibVar > MOVE_VIB_VAR || s_gyroEma > MOVE_GYRO_DPS;
  if (active == st.moving) { s_moveSince = 0; }
  else {
    if (!s_moveSince || s_moveCand != active) { s_moveSince = now; s_moveCand = active; }
    else if (now - s_moveSince >= MOVE_HOLD_MS) { st.moving = active; s_moveSince = 0; }
  }

  // Harsh events.
  bool fired = false;
  if (evCheck(s_evBrake,  st.longG < BRAKE_G, now))        { st.harshBrakeCount++;  fired = true; }
  if (evCheck(s_evAccel,  st.longG > ACCEL_G, now))        { st.harshAccelCount++;  fired = true; }
  if (evCheck(s_evCorner, fabsf(st.latG) > CORNER_G, now)) { st.harshCornerCount++; fired = true; }
  if (fired) st.lastEventMs = now;
}
