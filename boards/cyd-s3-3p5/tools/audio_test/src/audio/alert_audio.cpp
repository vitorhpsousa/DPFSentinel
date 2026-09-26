#include "alert_audio.h"
#include <Wire.h>
#include <math.h>
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const uint32_t SR = 16000;  // PROVEN: factory firmware uses 16 kHz/16-bit; MCLK = 256*SR = 4.096 MHz
static i2s_chan_handle_t s_tx = nullptr;
static QueueHandle_t s_q = nullptr;
static volatile bool s_ready = false;

struct Tone { uint16_t hz; uint16_t ms; uint8_t vol; };  // hz==0 => silence

static bool wr(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(AUDIO_ES8311_ADDR);
  Wire.write(reg); Wire.write(v);
  return Wire.endTransmission() == 0;
}
static bool rd(uint8_t reg, uint8_t &v) {
  Wire.beginTransmission(AUDIO_ES8311_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)AUDIO_ES8311_ADDR, (uint8_t)1) != 1) return false;
  v = Wire.read(); return true;
}

static void paSet(bool on) { digitalWrite(AUDIO_PIN_PA, (on == (AUDIO_PA_ACTIVE_HIGH != 0)) ? HIGH : LOW); }

// Register init follows Espressif's es8311.cpp (vendor Example_29_ai_chat), 16-bit I2S slave, MCLK from pin.
static bool codecInit() {
  uint8_t id; if (!rd(0xFD, id)) return false;  // chip id reg (ES8311 CHD1 = 0x83); ACK is the real check
  bool ok = true;
  ok &= wr(0x00, 0x1F); delay(20);
  ok &= wr(0x00, 0x00);
  ok &= wr(0x00, 0x80);            // power on, slave mode
  ok &= wr(0x01, 0x3F);            // all clocks on, MCLK from pin
  // Coefficients for MCLK 4.096 MHz / 16 kHz (table row: pre_div 1, mult 0, adc/dac_div 1, fs 0, lrck 0x00FF, bclk 4, osr 0x10)
  uint8_t r;
  if (!rd(0x02, r)) return false;
  r &= 0x07; r |= (1 - 1) << 5; r |= 0 << 3; ok &= wr(0x02, r);
  ok &= wr(0x03, 0x10);
  ok &= wr(0x04, 0x10);
  ok &= wr(0x05, 0x00);
  if (!rd(0x06, r)) return false;
  r &= 0xE0; r &= ~(1 << 5); r |= (4 - 1); ok &= wr(0x06, r);  // BCLK div 4, SCLK not inverted
  if (!rd(0x07, r)) return false;
  r &= 0xC0; r |= 0x00; ok &= wr(0x07, r);
  ok &= wr(0x08, 0xFF);
  ok &= wr(0x09, 0x0C);            // SDP in: 16-bit I2S
  ok &= wr(0x0A, 0x0C);            // SDP out: 16-bit
  ok &= wr(0x0D, 0x01);            // power up analog
  ok &= wr(0x0E, 0x02);
  ok &= wr(0x12, 0x00);            // power up DAC
  ok &= wr(0x13, 0x10);            // enable HP drive output
  ok &= wr(0x1C, 0x6A);
  ok &= wr(0x37, 0x08);            // bypass DAC EQ
  // DAC volume reg 0x32: codec-side ceiling. GUESS: 0xBF (~75%) leaves headroom; software scaling does the real capping.
  ok &= wr(0x32, 0xBF);
  if (rd(0x31, r)) ok &= wr(0x31, r & ~((1 << 6) | (1 << 5)));  // unmute
  return ok;
}

static void playTone(const Tone &t) {
  const int chunk = 256;  // frames per write
  static int16_t buf[256 * 2];
  uint32_t total = (uint32_t)SR * t.ms / 1000;
  uint8_t vol = t.vol > AUDIO_MAX_VOLUME_PCT ? AUDIO_MAX_VOLUME_PCT : t.vol;
  float amp = 32767.0f * 0.9f * vol / 100.0f;
  const uint32_t ramp = SR / 200;  // 5 ms fade in/out to avoid clicks
  float phase = 0, inc = t.hz ? 2.0f * (float)M_PI * t.hz / SR : 0;
  for (uint32_t n = 0; n < total;) {
    int cnt = (total - n) < (uint32_t)chunk ? (int)(total - n) : chunk;
    for (int i = 0; i < cnt; i++, n++) {
      float env = 1.0f;
      if (n < ramp) env = (float)n / ramp;
      else if (total - n < ramp) env = (float)(total - n) / ramp;
      int16_t s = t.hz ? (int16_t)(sinf(phase) * amp * env) : 0;
      phase += inc; if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
      buf[2 * i] = s; buf[2 * i + 1] = s;
    }
    size_t w = 0;
    i2s_channel_write(s_tx, buf, cnt * 4, &w, 1000);
  }
}

static void audioTask(void *) {
  Tone t; bool paOn = false;
  for (;;) {
    if (xQueueReceive(s_q, &t, paOn ? pdMS_TO_TICKS(300) : portMAX_DELAY) == pdTRUE) {
      if (!paOn) { paSet(true); paOn = true; delay(30); }  // let PA settle; GUESS 30 ms
      playTone(t);
    } else if (paOn) {  // idle 300 ms: flush silence, PA off to avoid hiss
      static int16_t z[128 * 2] = {0}; size_t w;
      i2s_channel_write(s_tx, z, sizeof z, &w, 100);
      paSet(false); paOn = false;
    }
  }
}

bool audioBegin() {
  if (s_ready) return true;
  pinMode(AUDIO_PIN_PA, OUTPUT); paSet(false);
  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  cc.auto_clear = true;
  if (i2s_new_channel(&cc, &s_tx, nullptr) != ESP_OK) return false;
  i2s_std_config_t sc = {};
  sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SR);  // mclk_multiple default 256
  sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  sc.gpio_cfg.mclk = (gpio_num_t)AUDIO_PIN_MCLK;
  sc.gpio_cfg.bclk = (gpio_num_t)AUDIO_PIN_BCLK;
  sc.gpio_cfg.ws = (gpio_num_t)AUDIO_PIN_WS;
  sc.gpio_cfg.dout = (gpio_num_t)AUDIO_PIN_DOUT;
  sc.gpio_cfg.din = (gpio_num_t)AUDIO_PIN_DIN;
  if (i2s_channel_init_std_mode(s_tx, &sc) != ESP_OK) return false;
  if (i2s_channel_enable(s_tx) != ESP_OK) return false;  // MCLK must run before codec config
  delay(10);
  if (!codecInit()) return false;
  s_q = xQueueCreate(16, sizeof(Tone));
  if (!s_q) return false;
  s_ready = true;
  xTaskCreate(audioTask, "audio", 4096, nullptr, 2, nullptr);
  return true;
}

void audioBeep(uint16_t hz, uint16_t ms, uint8_t volumePct) {
  if (!s_ready) return;
  Tone t{hz, ms, volumePct};
  xQueueSend(s_q, &t, 0);  // drops if queue full
}

// Tone sequences: all frequencies/timings are GUESSES chosen to be distinguishable; tune by ear.
void audioAlert(AlertKind kind) {
  const uint8_t v = AUDIO_MAX_VOLUME_PCT;
  switch (kind) {
    case REGEN_START: audioBeep(600, 120, v); audioBeep(900, 160, v); break;             // rising pair
    case REGEN_END:   audioBeep(900, 120, v); audioBeep(600, 160, v); break;             // falling pair
    case SOOT_AMBER:  audioBeep(880, 200, v); audioBeep(0, 100, 0); audioBeep(880, 200, v); break;
    case SOOT_RED:    for (int i = 0; i < 3; i++) { audioBeep(1200, 150, v); audioBeep(800, 150, v); } break;
    case TEST:        audioBeep(1000, 150, v); break;
  }
}
