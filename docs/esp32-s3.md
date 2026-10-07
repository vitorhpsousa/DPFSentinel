# ESP32-S3 (BLE) logger

An alternative to the Pi: an ESP32-S3 talks to a BLE ELM327 adapter, logs to microSD, serves a small web UI and can send Telegram messages. Same PID set and decoders as the Pi ([pid-map.md](pid-map.md)), different transport. Everything here is taken from the code in this repo's root; nothing was built or flashed while writing this page, so treat the build steps as untested by this draft. Telegram details are in [telegram-alerts.md](telegram-alerts.md). A future C3/C6 port is described in [esp32-c3-c6.md](esp32-c3-c6.md).

## Hardware

- An ESP32-S3 board. The code and pins target a Freenove ESP32-S3-WROOM with a built-in microSD slot (PlatformIO board `esp32-s3-devkitc-1`).
- A BLE ELM327 adapter that exposes a UART-style GATT service. Developed on a Konnwei; a code comment records that this adapter connected only 45 times in 992 attempts from a phone, so expect flakiness. Auto-detect prefers the Nordic UART service, then any service with a notify characteristic plus a writable one.
- microSD, FAT32 (8 GB was used). With `USE_SD 1` all files go under `/obd`; the firmware never formats the card (`SD_MMC.begin(..., false)` disables format-on-fail). If SD init fails it falls back to internal LittleFS (about 1.4 MB per the config comment), where the oldest session/raw files are deleted when space runs low.
- Optional LEDs: GPIO 1 = regen (on while `dpf_regen_active` is nonzero), GPIO 2 = hot warning (on when `egt_before_dpf_c`, else `dpf_zone_temp_c`, is at or above `DPF_TEMP_WARN_THRESHOLD_C`, 300 C). Use a series resistor. Powering the board in the car is not covered.

Built-in SD slot pins (`SD_USE_SDMMC 1`, 1-bit mode): CLK 39, CMD 38, D0 40. Alternative external SPI module (`SD_USE_SDMMC 0`): CS 10, MOSI 11, SCK 12, MISO 13.

## Build and flash

The toolchain is PlatformIO (`platformio.ini`: platform `espressif32`, Arduino framework, LittleFS, monitor 115200, library `h2zero/NimBLE-Arduino@^1.4.3`).

```bash
# 1. create your own src/config.h values (below)
pio run -t upload
pio device monitor        # opening the serial port RESETS the board and starts a new session
```

## Configuration (`src/config.h`)

Copy `src/config.example.h` to `src/config.h` and fill in your own Wi-Fi passwords and Telegram token/chat id there. `.gitignore` excludes `src/config.h`, so it stays untracked — never remove that exclusion or force-add the file. Settings by name:

| Setting | Meaning |
|---|---|
| `BLE_SCAN_ONLY` / `BLE_DUMP_SERVICES` | discovery aids, enable at most one. Scan lists every BLE device; dump connects and lists GATT services/characteristics |
| `BLE_TARGET_ADDRESS` | adapter address (from the scan); fastest connect |
| `BLE_NAME_HINT` | otherwise the name fragment to look for. If both are empty the name must contain one of: OBD, ELM, VLINK, V-LINK, VEEPEAK, IOS-, VGATE, KONNWEI, KW9, KW8, KW-, OBDLINK, MX+, LELink, CX. With several candidates the strongest signal wins |
| `BLE_SERVICE_UUID`, `BLE_CHAR_TX_UUID`, `BLE_CHAR_RX_UUID` | leave empty to auto-detect; the chosen UUIDs are printed on connect |
| `USE_SD`, `SD_USE_SDMMC`, pin macros | storage backend and pins |
| `WEB_ENABLED`, `WIFI_AP_ENABLED`, `WIFI_AP_SSID`, `WIFI_AP_PASS` | own hotspot for the web UI. `WEB_ENABLED 0` also disables the Telegram report task, because `reportBegin` is called inside that switch |
| `WIFI_STA_SSID/PASS`, `WIFI_STA_SSID2/PASS2` | up to two networks to join (e.g. home and a phone hotspot), tried alternately; leave the SSID empty to disable. 2.4 GHz only |
| `NTP_ENABLED` | set the clock from `pool.ntp.org` / `time.google.com` after joining |
| `TG_*` | Telegram token, chat id, report time, alerts, status pings. See [telegram-alerts.md](telegram-alerts.md) |
| `ROW_INTERVAL_MS` | one CSV row per interval (1000) with the latest non-stale values |
| `SERIAL_RAW_ECHO` | echo every raw adapter exchange on serial; bring-up only |
| `BOOST_UNIT` | unit of `turbo_boost` (always empty on the ix35) |
| `BLANK_ROWS_REINIT` / `BLANK_ROWS_RECONNECT` | 10 / 30 all-blank rows before re-running adapter init / reconnecting BLE |
| `DPF_IDLE_PRESSURE_THRESHOLD_HPA`, `DPF_IDLE_RPM_CEILING` | idle flag (20 hPa, 1100 rpm). It only prints a serial line; thresholds unverified; no warm-engine check |
| `DPF_TEMP_WARN_THRESHOLD_C`, `LED_REGEN_PIN`, `LED_TEMP_WARN_PIN` | LED behaviour |

The adapter init on the S3 is `ATZ`, then `ATE0`, `ATL0`, `ATH1`, `ATS0`, `ATSP6`, `ATST32` (200 ms reply timeout, vs 600 ms on the Pi).

## First connection

Plug the adapter into the OBD port with the ignition on and make sure no phone app is connected to it. Watch the serial monitor. If it is not found, set `BLE_SCAN_ONLY 1`, flash, read the address, then set `BLE_TARGET_ADDRESS` (or `BLE_NAME_HINT`) and turn the aid off again. After a first successful connect it reconnects by cached address and drops the cache after two consecutive failures.

## Scheduling and resilience

Each unique `(header, request)` gets a target period from `pid_registry.h` (RPM 200 ms, load 250, speed and MAF 500, DPF pressure and the `2103` block 1000, coolant/battery/EGR/soot 2000, intake air and EPS 5000; a shared request uses the shortest period). The loop serves the most overdue request, one adapter round trip at a time. After 5 consecutive failures a request is retried only every 30 s. A value older than `max(3 x period, 3 s)` is written as empty. If the link drops, a blank row is written and reconnection is attempted every 5 s.

## Web UI and API (no authentication)

Served on port 80 (Arduino `WebServer`). With the hotspot enabled, join `WIFI_AP_SSID`; the address is printed on serial at boot. On a joined network it is also announced as `obd-logger.local` (mDNS).

| Path | Purpose |
|---|---|
| `/` | single-page dashboard |
| `/api/live` | JSON: `t` (millis), `link`, `session`, `local` time, `time` source, `home` IP, `storage`, `free_mb`, and every column under `values` (null when empty) |
| `/api/files`, `/dl?f=session_N.csv` | list and download logs; names must start with `session_` or `raw_` |
| `/api/time?ms=<unix-ms>` | set the clock from a phone, only if not already valid |
| `/api/send` | queue the report now (includes small files) |
| `/api/report`, `?reset` | last report result; `reset` clears the sent-so-far bookkeeping |
| `/api/testalert` | queue a test alert |
| `/api/statusnow` | queue a status ping |

Anyone who can reach the board can download logs and trigger sends.

Wi-Fi behaviour (`web/web_ui.cpp`): the board tries a configured network for 20 s at boot, alternating between the two slots on later attempts. With the hotspot enabled it retries only every 10 minutes and only while the engine is off (RPM not above 0), to avoid disturbing the BLE link; with the hotspot disabled it retries every 30 s. Losing the network falls back to hotspot only. Timezone is hard-coded for the UK.

## Serial maintenance keys

Send over the serial monitor: `d` dump all files, `p` tail the previous boot's session (`bootNumber - 1`, 240 rows), `e` **erase every file** in the log folder (destructive).

## Known issues and limits

- Comments in `src/config.h` still say there is no wall clock; the code writes both `millis` and `unix_time`.
- `main.cpp` looks columns up with `colOf(name)`, which returns -1 for a missing name and is then used as an index. Do not remove columns without fixing this.
- The payload buffer per request is 96 bytes; a longer reply would be truncated. The largest one used (`2103`) needs at least 75 bytes.
- No automated tests are in the repository; the author reports `regen_watch` was replayed natively against the reference log, but that harness is not included.
- Sessions from two cars sharing one card share one boot counter.
