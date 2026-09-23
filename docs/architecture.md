# Architecture

Everything below was checked against the code in `pi/` and `esp32-s3-ble/`. Where an older note disagreed with the code, the code is described.

## Data flow

```
Car (ECM 7E0, steering ECU 7D4, CAN 11-bit 500 kbit/s)
   |  OBD-II
OBD adapter  (Pi: OBDLink MX+, Bluetooth Classic   |  S3: BLE ELM327)
   |  ASCII text, commands end with \r, replies end with ">"
Logger  (pi/main.py  |  esp32-s3-ble/src/main.cpp)
   |  1. select CAN header (ATSH)  2. send request + frame-count digit
   |  3. reassemble ISO-TP frames  4. check payload prefix  5. decode
   +--> session_N.csv   one row per second, decoded values
   +--> raw_N.log       every raw reply, unfiltered
   +--> Pi: webui/server.py (:8080) --> soot_panel.py (/dev/fb0)
   +--> S3: built-in web UI, Telegram
```

Steps in detail:

1. Requests and AT commands are ASCII, terminated by `\r`. The adapter replies and finishes with `>` (`pi/obd/elm_client.py: send_command`).
2. Headers are on (`ATH1`), so every CAN frame comes back as hex prefixed with its 3-character id, for example `7E8034104A2` (spaces off, `ATS0`).
3. `pi/obd/isotp.py` keeps only frames whose id equals the expected reply id and reassembles single frames (PCI 0) and first + consecutive frames (PCI 1 / 2) into a payload starting at the service byte (`0x41` for mode 01, `0x61` for mode 21). It returns `None` if the id is wrong or fewer than the announced bytes arrived. It does not check consecutive-frame sequence numbers.
4. A payload is accepted only if it starts with the PID's `prefix`; otherwise the cell is empty.
5. Each decoder reads fixed byte offsets and applies a scale. Requests shared by several columns (everything in the `2103` reply) go out once per cycle.

Adapter init (`ElmClient.init_adapter`): `ATZ`, then `ATE0`, `ATL0`, `ATH1`, `ATS0`, `ATSP6`, `ATST96`. `ATSP6` fixes the protocol to ISO 15765-4 CAN 11-bit/500k (the code comment says auto-search took 8 to 10 s and made every request time out). `ATST96` is a 600 ms reply timeout (0x96 x 4 ms). The S3 firmware uses `ATST32` (200 ms) instead. Each init command must return non-empty text or init is reported as failed.

CAN transmit headers and reply ids (`RX_ID` in `pi/obd/pid_registry.py`):

| Request header | Target | Reply id |
|---|---|---|
| `7DF` | functional broadcast, mode 01 | `7E8` |
| `7E0` | engine ECU, mode 21 | `7E8` |
| `7D4` | steering (EPS) ECU | `7DC` |

## Pi logger (`pi/main.py`)

- One pass over `PID_TABLE` per cycle, then a sleep of `POLL_INTERVAL_S` (1.0 s). The pass itself takes extra time, so rows are slightly slower than 1 Hz.
- Every failure (dropped link, failed reconnect, write timeout) is caught inside the loop. One blank row is written per attempt, the adapter is closed and reopened every 5 s (`RECONNECT_DELAY_S`). The process deliberately does not exit, because systemd restarting it produced hundreds of near-empty session files (comment in `main.py`).
- After `BLANK_STREAK_LIMIT = 10` consecutive cycles with no decoded value, it raises internally, which re-runs adapter init.
- Raw replies are always written while data flows. While nothing decodes, only the first blank cycle and every 60th after it are kept.
- `CALIBRATION_MODE = True` replaces normal polling with a raw dump of four requests to `calibration.csv` (`CALIBRATION_REQUESTS`).
- The idle-blockage heuristic (`pi/dpf/dpf_monitor.py`) is `rpm <= 1100 and pressure >= 20 hPa` (`DPF_IDLE_RPM_CEILING`, `DPF_IDLE_PRESSURE_THRESHOLD_HPA`). It does **not** check that the engine is warm. It prints a console line and the dashboard uses the same rule to flag a tile and mark a session summary. The thresholds are unverified.
- `dpf_odo_at_last_regen_mi` is derived (`odometer_mi - dpf_dist_since_regen_mi`), not requested.

## Files

Numbering: on the Pi `N` is the first unused `session_N.csv` in `LOG_DIR`. On the S3 `N` is a boot counter stored in flash (`Preferences`, namespace `obd`, key `boot`).

### `session_N.csv`

Empty cell = no valid value. Numbers are written with 3 decimals. The header row names every column.

Pi (`pi/storage/session_store.py`, `pi/obd/pid_registry.py`), 21 columns:

```
unix_time, engine_rpm, coolant_temp, vehicle_speed, maf_flow, engine_load_pct,
egr_duty_pct, intake_air_temp_c, dpf_zone_temp_c, dpf_diff_pressure_hpa,
dpf_soot_level_g, intercooler_temp_c, egt_before_dpf_c, dpf_dist_since_regen_mi,
odometer_mi, dpf_regen_active, dpf_regen_burning, eps_speed_kmh, eps_steering_deg,
eps_voltage_v, dpf_odo_at_last_regen_mi
```

`unix_time` is `time.time()` at write time, so it is only as good as the Pi's clock (see [pi-setup.md](pi-setup.md#clock)). Each row is `fsync`ed because a short cold trip can end in a power cut.

ESP32-S3 (`esp32-s3-ble/src/logging/session_store.cpp`, `obd/pid_registry.h`), 28 columns:

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
# Pi:  unix_time <TAB> header <TAB> request+digit <TAB> reply
1758000000.123    7E0    2103B    7E8104B6103...|7E821...|7E822...
# S3:  millis <TAB> header <TAB> request+digit <TAB> reply
48213    7DF    010C1    7E804410C1AF8
```

(Illustrative lines, not real captures; the first reply is shortened. If the ECU has nothing to say the reply text is an adapter message such as `NO DATA`; a failed header switch is logged as request `ATSH<hdr>`, reply `HEADER SWITCH FAILED` on the Pi.)

The S3 writes a special line when its clock is set: `millis`, header `TIME`, request = source (`ntp` or `phone`), reply = unix time. `tools/date_session.py` uses it to backfill the empty `unix_time` cells of earlier rows:

```
python3 tools/date_session.py session_33.csv raw_33.log > session_33_dated.csv
```

### `calibration.csv`

Pi only, calibration mode only: `unix_time, label, raw_response`.

## Clock

Neither device has a real-time clock. The Pi uses its system clock, which is unreliable in a car with no network and produced misleading symptoms (steps that looked like hangs). The S3 sets its clock at most once per boot, from NTP (`pool.ntp.org`, `time.google.com`, after joining Wi-Fi) or from the first phone that opens the web UI (`/api/time`), and rows before that carry only `millis`. The timezone string is hard-coded for the UK (`GMT0BST,M3.5.0/1,M10.5.0`) in `web/web_ui.cpp` and `web/timekeeper.cpp`.

## Services (Pi)

Units live in `pi/scripts/` and assume user `ix35` and the project at `/home/ix35/obd-logger`; edit them for your system.

| Unit | Runs | User | Notes |
|---|---|---|---|
| `obd-logger.service` | `scripts/bind_rfcomm.sh` (ExecStartPre), then `main.py` | root | `rfcomm bind` needs root; `Restart=on-failure`, `StartLimitIntervalSec=0` |
| `obd-dashboard.service` | `webui/server.py` | `ix35` | read-only against `LOG_DIR`, listens on `0.0.0.0:8080`, no authentication |
| `soot-panel.service` | `screen/soot_panel.py` | root | writes `/dev/fb0`; starts after `obd-dashboard.service` |

### Dashboard API (`pi/webui/server.py`)

| Path | Returns |
|---|---|
| `/` , `/app.js`, `/style.css` | the static single-page UI |
| `/api/live` | newest session by mtime: last 180 rows, latest row, `stale` (last row older than 8 s), `age_s`, `dpf_idle_flag` |
| `/api/sessions` | summary of every `session_*.csv` |
| `/api/sessions/session_N.csv` | one session, downsampled to at most 400 points, plus summary |
| `/api/schema` | **broken**: it reads `p.confirmed`, a field `PidDef` no longer has, so the request raises. The UI's "unconfirmed" hint (`app.js`) depends on it. Needs a small fix |

The server only reads CSVs; if it stops, logging is unaffected. It parses whole files on each request, which is fine for hobby volumes only.

### Soot panel (`pi/screen/soot_panel.py`)

Polls `http://localhost:8080/api/live` every second, renders 480x320 with Pillow and writes RGB565 to `/dev/fb0`, only when the frame bytes change.

| State | Display |
|---|---|
| no data, or `stale` | static gray screen: "NO LOGGER DATA" / "NO LIVE DATA" |
| `dpf_regen_active` set | near-black background, "ACTIVE REGENERATION / DO NOT SWITCH OFF / UNTIL IT FINISHES", soot, EGT before the DPF |
| otherwise | 2-column grid (soot, RPM, speed, coolant, diff pressure, catalyst temp, intercooler, MAF, since regen, regen state, odometer) |

Background colour: green below 14 g, amber 14 to 17 g, then a linear blend from amber to dark red (`#8b0000`) reaching full dark red at 28 g. These constants (`SOOT_GREEN_MAX`, `SOOT_AMBER_MAX`, `SOOT_DANGER_MAX`) are the owner's choices, not a specification. `python3 screen/soot_panel.py --demo` cycles fixed states every 30 s. The panel needs `requests` and `Pillow`, which are **not** in `pi/requirements.txt`, and the DejaVu fonts (`fonts-dejavu-core`).

## ESP32-S3

Same PID set and decoders, different transport. A tiered scheduler serves the most overdue request one at a time (periods 200 ms to 5 s, per-PID in `pid_registry.h`); after 5 consecutive failures a request is retried only every 30 s; a value older than `max(3 x period, 3 s)` is written empty; 10 all-blank rows re-run init, 30 reconnect BLE. Details, API and configuration: [esp32-s3.md](esp32-s3.md).
