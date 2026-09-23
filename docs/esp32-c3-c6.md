# ESP32-C3 and ESP32-C6

## Status: planned, not implemented

There is **no C3 or C6 code and no PlatformIO environment** in this repo.
The only firmware environments are `esp32-s3-ble/platformio.ini`
(`esp32-s3-devkitc-1`); `esp32/` and `astra/discovery/` are other S3-era
projects. Everything below is a plan plus reasoning. Nothing here has been run.

| Item | Status |
|---|---|
| ESP32-S3 firmware (`esp32-s3-ble/`) | Works (tested on the author's car) |
| Raspberry Pi 4 app (`pi/`) | Works (tested) |
| ESP32-C3 firmware / PlatformIO env | **Not implemented** |
| ESP32-C6 firmware / PlatformIO env | **Not implemented** |
| Round-screen UI (LVGL) | **Not implemented** |
| Porting `elm_client`/`isotp`/`pid_registry` to C3 | Untested (plan says it should need no changes) |
| Telegram on C3 within RAM | Untested, flagged as risk |
| Board pinout in this doc | Untested against real board (from a reference page) |
| C6 | Not implemented, not even planned in detail |

## The planned board (from the owner's notes, 2026-09-23)

One ESP32-C3 board with a built-in 1.28" round 240x240 GC9A01 screen ("No Touch"
variant), an AliExpress clone of the reference design "ESP32-2424S012"
(`https://homeding.github.io/boards/esp32c3/jczn-esp32-2424s012.htm`). Search that
name, not the seller's brand. It is intended to replace the Pi + screen + S3
split. The web dashboard (`WEB_ENABLED`, `WIFI_AP_ENABLED`, `web_ui.cpp`) is
dropped from that build, the round screen replaces it.

Pins from the reference page, **not confirmed on the physical board**:

- Display (SPI): SCK GPIO6, MOSI GPIO7, CS GPIO10, DC GPIO2, backlight GPIO3.
- Touch (CST816D, I2C, touch variants only): SDA 4, SCL 5, INT 0, RST 1. May not
  be exposed on the No-Touch variant.
- Button GPIO9.
- SD: needs an external SPI module; can share MOSI/SCK with the display, needs
  its own MISO and CS (candidates GPIO0/1/4/5/8, unconfirmed).
- Planned RTC: DS3231 on I2C (GPIO4/5), use a module without a
  non-rechargeable coin-cell charging circuit.

Verify pins with the board manual or a multimeter before wiring. Do not guess.

## What changes when porting `esp32-s3-ble`

Known differences (datasheet-level facts, plus marked reasoning):

- **Single core, RISC-V.** The S3 firmware pins tasks to core 0
  (`xTaskCreatePinnedToCore` in `report/telegram_report.cpp` and `web/web_ui.cpp`).
  The C3 has one core; pinning to core 0 works but the report/web tasks, the
  BLE stack and the OBD loop all compete for it. *Reasoning:* TLS handshakes and
  file uploads may stall the poll loop; needs measuring.
- **No PSRAM, about 400 KB SRAM** (from the notes). The S3 build uses a
  16 KB stack for the report task and String-based TLS uploads. *Reasoning:*
  NimBLE + LVGL frame buffers + SD + mbedTLS for Telegram may not fit together;
  the notes list this as the main open risk. Budget by measuring free heap.
- **BLE only.** ESP32-C3 (and C6) have no Bluetooth Classic. Only BLE ELM327
  adapters work (the same restriction as the S3, see the header of
  `esp32-s3-ble/src/config.h`). The OBDLink MX+ used with the Pi is Classic
  Bluetooth and will **not** work on these boards.
- **BLE library.** `esp32-s3-ble` uses `h2zero/NimBLE-Arduino@^1.4.3`; this
  should also support C3 (check the library's supported-chip list for the version you pin;
  the C6 needs a newer library and Arduino core than the platform used here, reasoning).
- **Storage, LittleFS/SD.** `USE_SD` with `SD_USE_SDMMC 1` uses the S3 board's SDMMC
  pins. The C3 has no SD_MMC host, so use SPI mode (`SD_USE_SDMMC 0`, pins
  `SD_CS_PIN` etc.). Fewer GPIOs, so check conflicts with the display.
- **Telegram/Wi-Fi.** Outbound HTTPS works over Wi-Fi STA; no AP needed
  (notes). Wi-Fi is 2.4 GHz on C3.
- **Config and LEDs.** `LED_REGEN_PIN 1` / `LED_TEMP_WARN_PIN 2` in `config.h` are
  Freenove S3 pins and clash with C3 pins in the plan (GPIO1 is a touch reset
  candidate, 2 is display DC). Remove or re-map.

### ESP32-C6 differences (datasheet facts)

Also RISC-V, single high-performance core, no PSRAM support on the module family
commonly used. Adds Wi-Fi 6 (802.11ax, 2.4 GHz) and an IEEE 802.15.4 radio
(Thread/Zigbee), BLE 5. None of these help this project: still BLE only, still
2.4 GHz only. *Reasoning:* the C6 has somewhat more SRAM than the C3
(check the datasheet for your module) so it may leave more headroom, but nothing has been tried.

## Porting checklist

1. Confirm the real board pinout (display, button, free GPIOs) with the manual or a
   multimeter.
2. Add a PlatformIO env for your board; copy `esp32-s3-ble/` and drop
   `src/web/` and the AP code (or keep the web UI if RAM allows).
3. Set BLE adapter fields (`BLE_TARGET_ADDRESS`, `BLE_NAME_HINT`) using
   `BLE_SCAN_ONLY` / `BLE_DUMP_SERVICES` on the new chip. Verify.
4. Re-map SD to SPI and pins; verify a card mounts and rows are written.
5. Keep `obd/elm_client.cpp`, `obd/isotp.cpp`, `obd/pid_registry.h`; check with
   the same PID output as the S3 on the same car.
6. Add the RTC or NTP time source; check `timeValid()` behaviour.
7. Build the display UI; measure free heap during a Telegram upload while BLE
   is polling (`ESP.getFreeHeap()` / min free heap).
8. Trim features (raw log upload, status pings, String buffers) until heap has margin.
9. Only then update the status table above.

Remember the PID map is car-specific (see `pid-map.md`); the port does not change that.
