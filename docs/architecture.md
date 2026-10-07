# Architecture

This repository (`obd-esp32`) ships the ESP32-S3 firmware only: everything
below describes its three targets (root build, `boards/cyd-s3-3p5/firmware/`,
`boards/waveshare-s3-touch-lcd-2/firmware/`). A Raspberry Pi 4 Python version
of the same logger exists as a separate, still-private project (`obd-pi`) that
predates the split of this repository from it; where this page used to
document that project's internals side by side with the firmware, it now
links to [cross-project-notes.md](cross-project-notes.md) instead — see that
page's disclaimer about what can and cannot be verified there.

## Data flow

```
Car (ECM 7E0, steering ECU 7D4, CAN 11-bit 500 kbit/s)
   |  OBD-II
OBD adapter  (BLE ELM327)
   |  ASCII text, commands end with \r, replies end with ">"
Logger  (src/main.cpp, per target)
   |  1. select CAN header (ATSH)  2. send request + frame-count digit
   |  3. reassemble ISO-TP frames  4. check payload prefix  5. decode
   +--> session_N.csv   one row per second, decoded values
   +--> raw_N.log       every raw reply, unfiltered
   +--> built-in web UI, Telegram
```

(The separate Pi project follows the same OBD-II/ELM327 approach over a
different adapter and transport, and additionally serves a Python web
dashboard and an SPI-screen renderer; see
[cross-project-notes.md](cross-project-notes.md#architecture-pi-logger-services-dashboard-soot-panel).)

Steps in detail:

1. Requests and AT commands are ASCII, terminated by `\r`. The adapter replies and finishes with `>`.
2. Headers are on (`ATH1`), so every CAN frame comes back as hex prefixed with its 3-character id, for example `7E8034104A2` (spaces off, `ATS0`).
3. ISO-TP reassembly keeps only frames whose id equals the expected reply id and reassembles single frames (PCI 0) and first + consecutive frames (PCI 1 / 2) into a payload starting at the service byte (`0x41` for mode 01, `0x61` for mode 21); fewer bytes than announced, or the wrong id, yields no value. Consecutive-frame sequence numbers are not checked.
4. A payload is accepted only if it starts with the PID's `prefix`; otherwise the cell is empty.
5. Each decoder reads fixed byte offsets and applies a scale. Requests shared by several columns (everything in the `2103` reply) go out once per cycle.

This repository's implementation is `src/obd/elm_client.cpp` + `src/obd/isotp.cpp`, duplicated per target at the repo root and under each `boards/*/firmware/src/obd/`.

Adapter init: `ATZ`, then `ATE0`, `ATL0`, `ATH1`, `ATS0`, `ATSP6`, `ATST32`. `ATSP6` fixes the protocol to ISO 15765-4 CAN 11-bit/500k (auto-search took 8 to 10 s on this car and made every request time out). `ATST32` is a 200 ms reply timeout. Each init command must return non-empty text or init is reported as failed.

CAN transmit headers and reply ids (see [pid-map.md](pid-map.md) for the full per-PID table):

| Request header | Target | Reply id |
|---|---|---|
| `7DF` | functional broadcast, mode 01 | `7E8` |
| `7E0` | engine ECU, mode 21 | `7E8` |
| `7D4` | steering (EPS) ECU | `7DC` |

## Files

`N` is a boot counter stored in flash (`Preferences`, namespace `obd`, key `boot`).

### `session_N.csv`

Empty cell = no valid value. Numbers are written with 3 decimals. The header row names every column.

ESP32-S3 (`src/logging/session_store.cpp`, `obd/pid_registry.h`), 28 columns:

```
millis, unix_time, engine_rpm, vehicle_speed, engine_load_pct, maf_flow,
intake_map_mbar, baro_kpa, coolant_temp, control_module_v, egr_duty_pct,
intake_air_temp_c, fuel_level_enh, fuel_rail_kpa, dpf_zone_temp_c,
dpf_diff_pressure_hpa, egt_before_dpf_c, dpf_dist_since_regen_mi, odometer_mi,
dpf_regen_active, dpf_regen_burning, dpf_soot_level_g, intercooler_temp_c,
eps_speed_kmh, eps_steering_deg, eps_voltage_v,
dpf_odo_at_last_regen_mi, turbo_boost
```

`intake_map_mbar`, `baro_kpa`, `fuel_level_enh` and `fuel_rail_kpa` are probe rows the ix35 does not answer (they stay empty); `turbo_boost` is derived from the first two and is therefore always empty on this car. `millis` is milliseconds since boot. `unix_time` is empty until the clock has been set (see below). The S3 does not fsync; it calls `flush()` after each row.

### `raw_N.log`

Tab-separated, one line per request, frames joined with `|`, always written unfiltered.

```
# millis <TAB> header <TAB> request+digit <TAB> reply
48213    7DF    010C1    7E804410C1AF8
```

(Illustrative line, not a real capture; the reply is shortened. If the ECU has nothing to say the reply text is an adapter message such as `NO DATA`.)

The S3 writes a special line when its clock is set: `millis`, header `TIME`, request = source (`ntp` or `phone`), reply = unix time. `tools/date_session.py` uses it to backfill the empty `unix_time` cells of earlier rows:

```
python3 tools/date_session.py session_33.csv raw_33.log > session_33_dated.csv
```

## Clock

The ESP32-S3 has no real-time clock. It sets its clock at most once per boot, from NTP (`pool.ntp.org`, `time.google.com`, after joining Wi-Fi) or from the first phone that opens the web UI (`/api/time`), and rows before that carry only `millis`. The timezone string is hard-coded for the UK (`GMT0BST,M3.5.0/1,M10.5.0`) in `web/web_ui.cpp` and `web/timekeeper.cpp`. (The separate Pi project has no RTC either, and relies on its own, unreliable system clock — see [cross-project-notes.md](cross-project-notes.md#architecture-pi-logger-services-dashboard-soot-panel).)

## ESP32-S3 polling and web UI

A tiered scheduler serves the most overdue request one at a time (periods 200 ms to 5 s, per-PID in `pid_registry.h`); after 5 consecutive failures a request is retried only every 30 s; a value older than `max(3 x period, 3 s)` is written empty; 10 all-blank rows re-run init, 30 reconnect BLE. Details, API and configuration: [esp32-s3.md](esp32-s3.md). The built-in web UI's `/api/live` and other endpoints are documented in [adapting/other-apps.md](adapting/other-apps.md). The two touchscreen boards' on-device UI is documented in [tech/screen-renderer.md](tech/screen-renderer.md).
