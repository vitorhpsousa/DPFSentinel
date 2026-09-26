#pragma once

// ESP32-S3 (WROOM-1) + BLE ELM327 adapter. Same PID set and decoders as
// pi/ (see obd/pid_registry.h), different transport. The S3 has BLE only, no
// Classic Bluetooth, which is what BLE ELM327 dongles use.
//
// No RTC on this build: rows are stamped with milliseconds since boot, and
// each boot gets its own session number (kept in flash), so sessions are
// ordered but not wall-clock dated. Line them up with other logs by shape.

// --- Discovery aids: enable at most ONE, flash, read Serial (115200) ---
// 1: scan and list every BLE device in range (name, address, services).
#define BLE_SCAN_ONLY 0
// 2: connect to the adapter and list every GATT service/characteristic.
#define BLE_DUMP_SERVICES 0

// --- Which adapter to connect to ---
// Fastest: the adapter's BLE address from BLE_SCAN_ONLY, e.g. "AA:BB:CC:DD:EE:FF".
#define BLE_TARGET_ADDRESS ""
// Otherwise the first device whose name contains this (case-insensitive) is
// used. If both are empty, names containing OBD / ELM / VLINK / VEEPEAK / IOS-
// are tried.
#define BLE_NAME_HINT ""

// GATT UART characteristics. Leave EMPTY to auto-detect (prefers the Nordic
// UART Service, then any service with a notify characteristic plus a
// writable one — covers FFF0/FFE0-style adapters). The chosen UUIDs are
// printed at connect time so you can pin them here afterwards.
#define BLE_SERVICE_UUID ""
#define BLE_CHAR_TX_UUID ""   // write: ESP32 -> adapter
#define BLE_CHAR_RX_UUID ""   // notify: adapter -> ESP32

// --- Storage ---
// 0 = internal flash (LittleFS, ~1.4 MB: about 30 minutes of driving).
// 1 = microSD. Files go in an /obd folder on the card; nothing else is touched.
#define USE_SD 1
// 1 = the board's built-in microSD slot (SD_MMC, 1-bit mode, Freenove
// ESP32-S3-WROOM pins below). 0 = an external SPI SD module on the SPI pins.
#define SD_USE_SDMMC 0   // Waveshare board: the slot is SPI (CS 41) on the LCD bus, see board/board.h
#define SDMMC_CLK_PIN 39
#define SDMMC_CMD_PIN 38
#define SDMMC_D0_PIN  40
#define SD_CS_PIN   41
#define SD_MOSI_PIN 38
#define SD_SCK_PIN  39
#define SD_MISO_PIN 40

// --- Wi-Fi hotspot + web dashboard ---
// The S3 hosts its own hotspot (no internet needed). Join it from a phone and
// open http://192.168.4.1 for live values and log downloads.
#define WEB_ENABLED 1
// 0 = join the home network only (no hotspot). 1 = also host the OBD-Logger hotspot.
#define WIFI_AP_ENABLED 0   // Waveshare board: the screen replaces the hotspot dashboard
#define WIFI_AP_SSID "OBD-Logger"
#define WIFI_AP_PASS "YOUR_WIFI_PASSWORD"
// Also join the home network when in range (reachable at http://obd-logger.local
// or the IP printed on Serial). Leave the SSID empty to run hotspot-only.
#define WIFI_STA_SSID "YOUR_WIFI_NAME"
#define WIFI_STA_PASS "YOUR_WIFI_PASSWORD"
// Second network to try (e.g. a phone hotspot) — tried alternately with the
// one above. Leave WIFI_STA_SSID2 empty to disable it.
#define WIFI_STA_SSID2 "YOUR_WIFI_NAME"
#define WIFI_STA_PASS2 "YOUR_WIFI_PASSWORD"
// Message Telegram with the network name and IP each time it joins one.
#define TG_ALERT_WIFI 1
// Set the clock from the internet when the home Wi-Fi is joined (no-op without it).
#define NTP_ENABLED 1

// --- Daily Telegram report ---
// Create a bot with @BotFather (it gives you the token), send the bot any
// message, then read your chat id from https://api.telegram.org/bot<TOKEN>/getUpdates
// (message.chat.id). Leave the token empty to switch this feature off.
#define TG_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define TG_CHAT_ID "YOUR_CHAT_ID"
// POSIX time-zone rule used for every "local time" in the firmware (the daily
// report time below, alert timestamps, the web UI). Daylight saving is handled
// automatically from the rule; nothing to change twice a year. Stored log
// times are always UTC and never affected.
// UK: "GMT0BST,M3.5.0/1,M10.5.0" = BST from the last Sunday of March (01:00 UTC)
// to the last Sunday of October (01:00 UTC). Other regions: look up your zone's
// POSIX TZ string (e.g. the posix_tz_db list) and paste it here.
#define TZ_RULE "GMT0BST,M3.5.0/1,M10.5.0"

#define TG_SEND_HOUR 17          // local time; sends at the first chance after this
#define TG_SEND_MIN 30
#define TG_SEND_RAW 1            // also upload the raw_N.log files
#define TG_MIN_BYTES 3000        // skip blank sessions (no adapter, no data)
#define TG_ALERT_REGEN 1        // message when a DPF regeneration starts / finishes
// Periodic status ping while online — fires only while actually connected, so
// no backlog of these builds up while it's offline. This will send one every
// 5 min for as long as the board sits connected, even parked — set 0 to
// switch it off, or make it fire less often.
#define TG_STATUS_ENABLED 1
#define TG_STATUS_INTERVAL_MIN 5
// Only sends while the engine is actually running and warm — skips cold
// starts, idling-while-parked, and every minute the board just sits
// connected doing nothing.
#define TG_STATUS_MIN_COOLANT_C 80.0f
#define TG_HOST "api.telegram.org"
#define TG_PORT 443
#define TG_USE_TLS 1             // 0 only for testing against a local stand-in server

// --- Behaviour ---
#define ROW_INTERVAL_MS 1000       // one CSV row per second (latest values held)
#define SERIAL_RAW_ECHO 1          // print every raw adapter exchange (bring-up)
#define BOOST_UNIT 0               // turbo_boost unit: 0 = kPa, 1 = PSI, 2 = mbar

// Consecutive all-blank rows before re-initialising the adapter, and before a
// full BLE reconnect. On the Pi an adapter stayed connected but useless for
// hours; this makes that self-healing.
#define BLANK_ROWS_REINIT 10
#define BLANK_ROWS_RECONNECT 30

// DPF idle-blockage heuristic (differential pressure while at idle).
#define DPF_IDLE_PRESSURE_THRESHOLD_HPA 20.0f
#define DPF_IDLE_RPM_CEILING            1100

// Status LEDs. GPIO 1 and 2 are free on the Freenove board (the camera uses
// 4-18, the SD slot 38-40, 26-32 are flash, 33-37 PSRAM).
#define LED_REGEN_PIN -1   // no LEDs on the Waveshare board (GPIO 1 is the backlight)
#define LED_TEMP_WARN_PIN -1
#define DPF_TEMP_WARN_THRESHOLD_C 300.0f

// Waveshare ESP32-S3-Touch-LCD-2 build: draws the soot screen (src/ui) and shares SPI with the SD card.
#define BOARD_WAVESHARE_LCD 1

// 1 = periodically ask the SD card for its free space (walks the FAT; can be slow). 0 = skipped for the
// car trial (2026-09-25): the system page then shows 0 MB free. Set back to 1 after the trial.
#define SD_FREE_CHECK 0

// Sleep when parked (Waveshare build): after SLEEP_IDLE_S seconds with no movement, no engine RPM and no touch,
// the screen, WiFi and BLE are switched off and the board naps in 250 ms light-sleep cycles, watching the motion
// sensor and touch. Movement (> SLEEP_WAKE_G change) or a touch wakes it with a software restart. The USB serial
// port disappears while it sleeps.
#define SLEEP_ENABLED 1
#define SLEEP_IDLE_S 180
#define SLEEP_WAKE_G 0.12f

// DIAGNOSTIC (2026-09-26): 1 = never start WiFi, to see whether WiFi scanning/joining is what stops the BLE adapter connecting.
#define WIFI_DISABLED 0
