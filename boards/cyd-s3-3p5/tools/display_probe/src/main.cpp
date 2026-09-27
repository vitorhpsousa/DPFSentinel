// Display + touch probe, 3.5" ESP32-S3 (ST77922, 320x480, QSPI).  NOT FLASHED YET.
//
// Pins used (only these; all from vendor/config.h + HARDWARE.md):
//   LCD: CS10 CLK12 D0=11 D1=13 D2=14 D3=9, BL=41 (active high), no RST pin
//   Touch: I2C SDA38 SCL39 addr 0x55, RST 48, INT 47
// SD and audio pins are deliberately untouched (note: I2C 38/39 is shared with ES8311 @0x18).
//
// Bus method: the ST77922's own QSPI command scheme (cmd 0x02 for register writes with the reg
// number in bits 8..15 of a 24-bit address, cmd 0x32 + addr 0x003C00 then quad pixel data for
// RAMWR) — this shape is dictated by the chip, cross-checked against LCDwiki's ydedox/st77922
// Example_01_Simple_test. Arduino_GFX has no ST77922 class, so text is drawn with Adafruit
// GFXcanvas16 in PSRAM and pushed as one full frame.
#include <Arduino.h>
#include <Wire.h>
#include <SD_MMC.h>
#include <Adafruit_GFX.h>
#include "driver/spi_master.h"
#include "hal/gpio_ll.h"

#define LCD_W 320
#define LCD_H 480
#define PIN_CS 10
#define PIN_CLK 12
#define PIN_D0 11
#define PIN_D1 13
#define PIN_D2 14
#define PIN_D3 9
#define PIN_BL 41
#define PIN_SDA 38
#define PIN_SCL 39
#define PIN_TRST 48
#define PIN_TINT 47
#define TOUCH_ADDR 0x55

#define QSPI_HZ 40000000  // 40 MHz probed OK by jlmeredith (ceiling 62.5); vendor example uses 80 MHz (out of spec). UNVERIFIED here.
#define TX_LEN 0x4000     // pixels per transaction

struct InitCmd { uint8_t cmd; const uint8_t *data; uint8_t len; uint16_t delay_ms; };
#define C(...) (const uint8_t[]){__VA_ARGS__}
// Table decoded straight from this board's own factory firmware (../../firmware_analysis.md): the
// vendor image's own st77922_vendor_config_t / init command array, 56 entries up to RASET, plus the
// tail this panel's factory app actually runs (INVON, SLPOUT/120ms, DISPON, RAMWR, COLMOD 01, MADCTL 00, TEON).
// This is the panel's own bring-up sequence, extracted from a flash image of the exact unit in hand —
// not copied from any third-party example.
static const InitCmd initTable[] = {
  {0xF1, C(0x00), 1, 0},
  {0x60, C(0x00,0x00,0x00), 3, 0},
  {0x65, C(0x80), 1, 0},
  {0x79, C(0x06), 1, 0},
  {0x7B, C(0x00,0x08,0x08), 3, 0},
  {0x80, C(0x55,0x62,0x2F,0x17,0xF0,0x52,0x70,0xD2,0x52,0x62,0xEA), 11, 0},
  {0x81, C(0x26,0x52,0x72,0x27), 4, 0},
  {0x84, C(0x92,0x25), 2, 0},
  {0x87, C(0x10,0x10,0x58,0x00,0x02,0x3A), 6, 0},
  {0x88, C(0x00,0x00,0x2C,0x10,0x04,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x06), 15, 0},
  {0x89, C(0x00,0x00,0x00), 3, 0},
  {0x8A, C(0x13,0x00,0x2C,0x00,0x00,0x2C,0x10,0x10,0x00,0x3E,0x19), 11, 0},
  {0x8B, C(0x15,0xB1,0xB1,0x44,0x96,0x2C,0x10,0x97,0x8E), 9, 0},
  {0x8C, C(0x1D,0xB1,0xB1,0x44,0x96,0x2C,0x10,0x50,0x0F,0x01,0xC5,0x12,0x09), 13, 0},
  {0x8D, C(0x0C), 1, 0},
  {0x8E, C(0x33,0x01,0x0C,0x13,0x01,0x01), 6, 0},
  {0xB3, C(0x00,0x30), 2, 0},
  {0xF1, C(0x00), 1, 0},
  {0x71, C(0xD0), 1, 0},
  {0x66, C(0x02,0x3F), 2, 0},
  {0xBE, C(0x26,0x00,0x9D), 3, 0},
  {0x70, C(0x01,0xA6,0x11,0x40,0xE0,0x00,0x11,0x60,0x11,0x00,0x00,0x1A), 12, 0},
  {0x90, C(0x04,0x04,0x55,0x74,0x00,0x40,0x43,0x2D,0x2D), 9, 0},
  {0x91, C(0x04,0x04,0x55,0x75,0x00,0x40,0x42,0x2D,0x2D), 9, 0},
  {0x92, C(0x04,0x44,0x55,0xC0,0x06,0x00,0x07,0x05,0x90,0x2D), 10, 0},
  {0x93, C(0x04,0x43,0x11,0x00,0x00,0x00,0x00,0x05,0x90,0x2D), 10, 0},
  {0x94, C(0x00,0x00,0x00,0x00,0x00,0x00), 6, 0},
  {0x95, C(0x96,0x16,0x00,0x00,0xFF), 5, 0},
  {0x96, C(0x44,0x53,0x03,0x12,0x23,0x24,0x06,0x05,0x9A,0x2D,0x00,0x44), 12, 0},
  {0x97, C(0x44,0x53,0x47,0x56,0x20,0x20,0x02,0x01,0x9A,0x2D,0x00,0x44), 12, 0},
  {0xBA, C(0x55,0x9A,0x2D,0x9A,0x2D), 5, 0},
  {0x9A, C(0x40,0x00,0x06,0x00,0x00,0x00,0x00), 7, 0},
  {0x9B, C(0x00,0x00,0x06,0x00,0x00,0x00,0x00), 7, 0},
  {0x9C, C(0x5C,0x12,0x00,0x00,0x10,0x12,0x00,0x00,0x10,0x02,0x00,0x00,0x00), 13, 0},
  {0x9D, C(0x8A,0x51,0x00,0x00,0x00,0x80,0x1E,0x01), 8, 0},
  {0x9E, C(0x51,0x00,0x00,0x00,0x80,0x1E,0x01), 7, 0},
  {0xB4, C(0x1D,0x1C,0x1E,0x0B,0x14,0x02,0x13,0x09,0x1E,0x00,0x1E,0x10), 12, 0},
  {0xB5, C(0x1D,0x1C,0x1E,0x0A,0x15,0x03,0x11,0x08,0x1E,0x01,0x1E,0x12), 12, 0},
  {0xB6, C(0x77,0x77,0x00,0x0A,0xFF,0x0A,0xFF), 7, 0},
  {0x86, C(0xC6,0x04,0xB1,0x02,0x58,0x12,0x58,0x0C,0x13,0x01,0xA5,0x00,0xA5,0xA5), 14, 0},
  {0xB7, C(0x07,0x0A,0x0E,0x06,0x05,0x03,0x2B,0x03,0x03,0x42,0x07,0x10,0x10,0x2E,0x3F,0x0D), 16, 0},
  {0xB8, C(0x07,0x0A,0x0D,0x05,0x05,0x02,0x2B,0x02,0x03,0x42,0x06,0x10,0x0F,0x2E,0x3F,0x0D), 16, 0},
  {0xB9, C(0x23,0x23), 2, 0},
  {0xBF, C(0x10,0x14,0x14,0x0B,0x0B,0x0B), 6, 0},
  {0xF2, C(0x00), 1, 0},
  {0x73, C(0x04,0xDA,0x12,0x54,0x47), 5, 0},
  {0x77, C(0x6B,0x5B,0xFD,0xC3,0xC5), 5, 0},
  {0x7A, C(0x15,0x27), 2, 0},
  {0x7B, C(0x04,0x57), 2, 0},
  {0x7E, C(0x01,0x0E), 2, 0},
  {0xBF, C(0x36), 1, 0},
  {0xE3, C(0x40,0x40), 2, 0},
  {0xF0, C(0x00), 1, 0},
  {0xD0, C(0x00), 1, 0},
  {0x2A, C(0x00,0x00,0x01,0x3F), 4, 0},
  {0x2B, C(0x00,0x00,0x01,0xDF), 4, 0},
  // tail
  {0x21, nullptr, 0, 0},          // INVON (vendor tables do this; jlmeredith says one stack needs invert, other not: colours may look inverted)
  {0x11, nullptr, 0, 120},        // SLPOUT
  {0x29, nullptr, 0, 0},          // DISPON
  {0x2C, nullptr, 0, 0},          // RAMWR
  {0x3A, C(0x01), 1, 0},          // COLMOD (vendor writes 01, panel also takes 55; RGB565 assumed)
  {0x36, C(0x00), 1, 0},          // MADCTL portrait. Colour order (BGR per xiaozhi config) UNVERIFIED
  {0x35, C(0x01), 1, 20},         // TEON (TE pin 42 is NOT used/driven by this probe)
};

static spi_device_handle_t qspi;

static inline void csLow()  { GPIO.out_w1tc = (1u << PIN_CS); }
static inline void csHigh() { GPIO.out_w1ts = (1u << PIN_CS); }

static void lcdReg(uint8_t cmd, const uint8_t *data, uint8_t len) {
  csLow();
  spi_transaction_ext_t t; memset(&t, 0, sizeof(t));
  t.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
  t.base.cmd = 0x02;
  t.base.addr = (uint32_t)cmd << 8;
  t.command_bits = 8;
  t.address_bits = 24;
  if (len) { t.base.tx_buffer = data; t.base.length = 8 * len; }
  spi_device_polling_transmit(qspi, (spi_transaction_t *)&t);
  csHigh();
}

static void lcdPushFrame(const uint16_t *px /* byte-swapped RGB565, LCD_W*LCD_H */) {
  const uint8_t xa[4] = {0, 0, (LCD_W - 1) >> 8, (LCD_W - 1) & 0xFF};
  const uint8_t ya[4] = {0, 0, (LCD_H - 1) >> 8, (LCD_H - 1) & 0xFF};
  lcdReg(0x2A, xa, 4);
  lcdReg(0x2B, ya, 4);
  size_t total = (size_t)LCD_W * LCD_H;
  bool first = true;
  spi_transaction_ext_t t; memset(&t, 0, sizeof(t));
  csLow();
  while (total) {
    if (first) {
      t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;
      t.base.cmd = 0x32; t.base.addr = 0x3C << 8;
      t.command_bits = 8; t.address_bits = 24;
      first = false;
    } else {
      // continuation chunks: no cmd/addr/dummy phases (same as vendor example)
      t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;
      t.command_bits = 0; t.address_bits = 0; t.dummy_bits = 0;
    }
    size_t n = total > TX_LEN ? TX_LEN : total;
    t.base.tx_buffer = px;
    t.base.length = n * 16;
    spi_device_polling_transmit(qspi, (spi_transaction_t *)&t);
    px += n; total -= n;
  }
  csHigh();
}

static GFXcanvas16 *canvas;
static uint16_t *txbuf;

static void present() {
  const uint16_t *src = canvas->getBuffer();
  for (size_t i = 0; i < (size_t)LCD_W * LCD_H; i++) txbuf[i] = __builtin_bswap16(src[i]);
  lcdPushFrame(txbuf);
}

static void lcdInit() {
  pinMode(PIN_CS, OUTPUT); digitalWrite(PIN_CS, HIGH);
  pinMode(PIN_BL, OUTPUT); digitalWrite(PIN_BL, LOW);  // backlight on only after first frame
  spi_bus_config_t bus = {};
  bus.data0_io_num = PIN_D0; bus.data1_io_num = PIN_D1; bus.sclk_io_num = PIN_CLK;
  bus.data2_io_num = PIN_D2; bus.data3_io_num = PIN_D3;
  bus.max_transfer_sz = TX_LEN * 2 + 8;  // bytes per transaction (vendor uses 16x that; DMA chunks anyway) UNVERIFIED
  bus.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD;  // IOMUX flag dropped: GPIO matrix is fine at 40 MHz
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
  spi_device_interface_config_t dev = {};
  dev.mode = 0; dev.clock_speed_hz = QSPI_HZ; dev.spics_io_num = -1;  // CS driven manually as in vendor code? see below
  dev.flags = SPI_DEVICE_HALFDUPLEX; dev.queue_size = 4;
  // Vendor example passes spics_io_num=10 AND toggles CS by GPIO register; we pass -1 and toggle by hand
  // to hold CS low across multi-chunk pixel writes. UNVERIFIED that this is equivalent.
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &qspi));
  for (size_t i = 0; i < sizeof(initTable) / sizeof(initTable[0]); i++) {
    lcdReg(initTable[i].cmd, initTable[i].data, initTable[i].len);
    if (initTable[i].delay_ms) delay(initTable[i].delay_ms);
  }
}

static void fillShow(uint16_t color, const char *name) {
  canvas->fillScreen(color);
  canvas->setTextColor(0xFFFF); canvas->setTextSize(4);
  canvas->setCursor(20, 200); canvas->print(name);
  present();
  Serial.printf("filled %s\n", name);
}

// ---------- touch ----------
static void i2cScan() {
  Serial.println("I2C scan (SDA38/SCL39):");
  int n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf("  found 0x%02X\n", a); n++; }
  }
  Serial.printf("  %d device(s)\n", n);
}

// Raw read: write regBytes (1 or 2 bytes) then read 8 bytes. No layout assumed.
static void rawRead(const char *label, const uint8_t *reg, uint8_t rlen, uint8_t *out) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(reg, rlen);
  int e = Wire.endTransmission(false);
  int got = e == 0 ? Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)8) : 0;
  memset(out, 0xEE, 8);
  for (int i = 0; i < got && Wire.available(); i++) out[i] = Wire.read();
  Serial.printf("  %s err=%d got=%d :", label, e, got);
  for (int i = 0; i < 8; i++) Serial.printf(" %02X", out[i]);
  Serial.println();
}

// SD card in 4-bit SDMMC mode: pins proven in the board's factory firmware (see ../firmware_analysis.md).
static void sdTest() {
  SD_MMC.setPins(5 /*CLK*/, 4 /*CMD*/, 6 /*D0*/, 7 /*D1*/, 2 /*D2*/, 3 /*D3*/);
  if (!SD_MMC.begin("/sdcard", false /*4-bit*/, false /*never format*/)) { Serial.println("SD: mount FAILED (4-bit); trying 1-bit"); SD_MMC.setPins(5, 4, 6); if (!SD_MMC.begin("/sdcard", true, false)) { Serial.println("SD: mount FAILED (1-bit too): card in? FAT32?"); return; } }
  uint8_t t = SD_MMC.cardType();
  Serial.printf("SD: type %s, %llu MB\n", t == CARD_MMC ? "MMC" : t == CARD_SD ? "SDSC" : t == CARD_SDHC ? "SDHC" : "?", SD_MMC.cardSize() / (1024ULL * 1024ULL));
  File f = SD_MMC.open("/probe.txt", FILE_WRITE);
  if (f) { f.print("probe ok"); f.close(); }
  f = SD_MMC.open("/probe.txt");
  String back = f ? f.readString() : "";
  if (f) f.close();
  SD_MMC.remove("/probe.txt");
  Serial.println(back == "probe ok" ? "SD: write+read OK" : "SD: write/read FAILED");
  SD_MMC.end();
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("display_probe start");

  pinMode(PIN_TINT, INPUT);
  Wire.begin(PIN_SDA, PIN_SCL, 400000);   // vendor firmware runs the touch device at 400 kHz
  sdTest();
  i2cScan();

  canvas = new GFXcanvas16(LCD_W, LCD_H);       // uses ps_malloc/malloc; 300 KB
  txbuf = (uint16_t *)heap_caps_malloc((size_t)LCD_W * LCD_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT /* PSRAM: internal RAM too small; vendor also uses ps_malloc. UNVERIFIED at runtime */);
  if (!canvas || !canvas->getBuffer() || !txbuf) {
    Serial.println("ALLOC FAILED (canvas/txbuf)");
    while (1) delay(1000);
  }
  lcdInit();
  // Vendor order (lcdwiki-es3c35p.cc): the LCD is initialised FIRST, then the touch controller is reset
  // (10 ms low, 100 ms settle) because the controller is integrated with the display driver.
  pinMode(PIN_TRST, OUTPUT); digitalWrite(PIN_TRST, LOW); delay(10);
  digitalWrite(PIN_TRST, HIGH); delay(100);
  fillShow(0xF800, "RED");
  digitalWrite(PIN_BL, HIGH);  // active high per vendor config
  delay(1500);
  fillShow(0x07E0, "GREEN"); delay(1500);
  fillShow(0x001F, "BLUE");  delay(1500);
  canvas->fillScreen(0x0000);
  canvas->setTextColor(0xFFFF); canvas->setTextSize(3);
  canvas->setCursor(10, 20);  canvas->print("ST77922 probe");
  canvas->setTextSize(2);
  canvas->setCursor(10, 70);  canvas->print("Touch the screen.");
  canvas->setCursor(10, 100); canvas->print("Raw bytes go to Serial.");
  canvas->drawRect(0, 0, LCD_W, LCD_H, 0xFFFF);  // frame: shows orientation/edges
  canvas->fillRect(0, 0, 20, 20, 0xF800);        // top-left red marker
  present();
  Serial.println("Touch dump: prints on change of raw bytes, plus 1s heartbeat with INT level.");
}

// Touch: Sitronix-style controller, 16-bit big-endian register addresses (vendor code lcdwiki-es3c35p.cc):
// reg 0x0010 bit 0x08 = touch present; reg 0x0014, 7 bytes per point: b0&0x80 valid, x=((b0&0x3F)<<8)|b1, y=((b2&0x3F)<<8)|b3.
static bool touchRead16(uint16_t reg, uint8_t *d, size_t n) {
  Wire.beginTransmission(TOUCH_ADDR); Wire.write((uint8_t)(reg >> 8)); Wire.write((uint8_t)(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)TOUCH_ADDR, (uint8_t)n) != n) return false;
  for (size_t i = 0; i < n; i++) d[i] = Wire.read();
  return true;
}

void loop() {
  static uint32_t lastLog = 0, lastPoll = 0;
  if (millis() - lastPoll < 30) return;
  lastPoll = millis();
  // ESPHome st7123 driver: ONE burst read from 0x0010 through the last touch point (4 + 5*7 bytes); the read
  // itself clears the INT line so the controller reports the next frame. Splitting it into two reads never
  // acknowledged the touch, so only the first touch ever registered.
  uint8_t d[4 + 5 * 7] = {0};
  bool ok = touchRead16(0x0010, d, sizeof d);
  int nTouch = 0;
  for (int i = 0; ok && i < 5; i++) {
    const uint8_t *p = d + 4 + i * 7;
    if (!(p[0] & 0x80)) continue;
    int x = ((p[0] & 0x3F) << 8) | p[1], y = ((p[2] & 0x3F) << 8) | p[3];
    nTouch++;
    canvas->fillCircle(x, y, 6, 0xFFE0);
    present();
    if (millis() - lastLog > 100) { lastLog = millis(); Serial.printf("TOUCH #%d x=%d y=%d\n", i, x, y); }
  }
  static uint32_t lastBeat = 0;
  if (millis() - lastBeat > 2000) { lastBeat = millis(); Serial.printf("beat ok=%d info=%02X INT=%d touches=%d\n", ok, d[0], digitalRead(PIN_TINT), nTouch); }
}
