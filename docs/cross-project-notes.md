# Cross-project notes (the Raspberry Pi companion project)

This page collects everything in this repository's docs that used to describe
**`obd-pi`**: a Raspberry Pi 4 Python version of this same DPF logger. Before
2026, the Pi app and this ESP32-S3 firmware were developed together in one
project; they were later split into two repositories. This one (`obd-esp32`)
is public. `obd-pi` is **separate and still private** — it is not published
anywhere this reader can open.

**Read this with that in mind:**

- Nothing on this page can be checked against real source by anyone outside
  the project. Every file path, line number and behaviour below is carried
  over from notes written while the Pi and S3 code lived side by side, not
  verified against this repository's own code.
- None of the Pi file paths mentioned below (`main.py`, `webui/server.py`,
  `screen/soot_panel.py`, `obd/pid_registry.py`, `obd/isotp.py`,
  `obd/elm_client.py`, `obd/ble_transport.py`, `alerts/regen_watch.py`,
  `alerts/telegram_alerts.py`, `scripts/*.service`, `storage/session_store.py`,
  `config.py`/`config_local.py`, `tools/carscanner_parse.py`,
  `tools/carscanner_solve.py`) exist in this repository. This repository is
  PlatformIO/C++ only: the root `src/` plus `boards/*/firmware/src/`.
- This page is not maintained against the Pi project's current code. Treat it
  as historical background, not documentation you can rely on.

Every other page in `docs/` that touches on the Pi project now links here
instead of repeating this detail inline.

---

## Architecture: Pi logger, services, dashboard, soot panel

*(moved from `docs/architecture.md`)*

**Pi logger (`main.py`):**

- One pass over `PID_TABLE` per cycle, then a sleep of `POLL_INTERVAL_S` (1.0 s). The pass itself takes extra time, so rows are slightly slower than 1 Hz.
- Every failure (dropped link, failed reconnect, write timeout) is caught inside the loop. One blank row is written per attempt, the adapter is closed and reopened every 5 s (`RECONNECT_DELAY_S`). The process deliberately does not exit, because systemd restarting it produced hundreds of near-empty session files (comment in `main.py`).
- After `BLANK_STREAK_LIMIT = 10` consecutive cycles with no decoded value, it raises internally, which re-runs adapter init.
- Raw replies are always written while data flows. While nothing decodes, only the first blank cycle and every 60th after it are kept.
- `CALIBRATION_MODE = True` replaces normal polling with a raw dump of four requests to `calibration.csv` (`CALIBRATION_REQUESTS`).
- The idle-blockage heuristic (`dpf/dpf_monitor.py`) is `rpm <= 1100 and pressure >= 20 hPa` (`DPF_IDLE_RPM_CEILING`, `DPF_IDLE_PRESSURE_THRESHOLD_HPA`). It does **not** check that the engine is warm. It prints a console line and the dashboard uses the same rule to flag a tile and mark a session summary. The thresholds are unverified.
- `dpf_odo_at_last_regen_mi` is derived (`odometer_mi - dpf_dist_since_regen_mi`), not requested.

**`session_N.csv` (Pi, `storage/session_store.py` + `obd/pid_registry.py`), 21 columns:**

```
unix_time, engine_rpm, coolant_temp, vehicle_speed, maf_flow, engine_load_pct,
egr_duty_pct, intake_air_temp_c, dpf_zone_temp_c, dpf_diff_pressure_hpa,
dpf_soot_level_g, intercooler_temp_c, egt_before_dpf_c, dpf_dist_since_regen_mi,
odometer_mi, dpf_regen_active, dpf_regen_burning, eps_speed_kmh, eps_steering_deg,
eps_voltage_v, dpf_odo_at_last_regen_mi
```

`unix_time` is `time.time()` at write time, so it is only as good as the Pi's own system clock, which has no RTC backing it in the car. Each row is `fsync`ed because a short cold trip can end in a power cut. Numbering: `N` is the first unused `session_N.csv` in `LOG_DIR`.

`raw_N.log` (Pi): tab-separated, one line per request, frames joined with `|`:

```
# Pi:  unix_time <TAB> header <TAB> request+digit <TAB> reply
1758000000.123    7E0    2103B    7E8104B6103...|7E821...|7E822...
```

(Illustrative line, not a real capture.) A failed header switch is logged as request `ATSH<hdr>`, reply `HEADER SWITCH FAILED` on the Pi.

`calibration.csv` (Pi only, calibration mode only): `unix_time, label, raw_response`.

**Clock:** the Pi has no real-time clock and uses its system clock, which is unreliable in a car with no network and produced misleading symptoms (steps that looked like hangs).

**Services (Pi):** units live in `scripts/` and assume user `ix35` and the project at `/home/ix35/obd-logger`.

| Unit | Runs | User | Notes |
|---|---|---|---|
| `obd-logger.service` | `scripts/bind_rfcomm.sh` (ExecStartPre), then `main.py` | root | `rfcomm bind` needs root; `Restart=on-failure`, `StartLimitIntervalSec=0` |
| `obd-dashboard.service` | `webui/server.py` | `ix35` | read-only against `LOG_DIR`, listens on `0.0.0.0:8080`, no authentication |
| `soot-panel.service` | `screen/soot_panel.py` | root | writes `/dev/fb0`; starts after `obd-dashboard.service` |

**Dashboard API (`webui/server.py`):**

| Path | Returns |
|---|---|
| `/` , `/app.js`, `/style.css` | the static single-page UI |
| `/api/live` | newest session by mtime: last 180 rows, latest row, `stale` (last row older than 8 s), `age_s`, `dpf_idle_flag` |
| `/api/sessions` | summary of every `session_*.csv` |
| `/api/sessions/session_N.csv` | one session, downsampled to at most 400 points, plus summary |
| `/api/schema` | returns `{"pids": [...], "dpf_idle_pressure_threshold_hpa", "dpf_idle_rpm_ceiling", "dpf_temp_warn_threshold_c"}` |

The server only reads CSVs; if it stops, logging is unaffected. It parses whole files on each request, which is fine for hobby volumes only.

**Soot panel (`screen/soot_panel.py`):** polls `http://localhost:8080/api/live` every second, renders 480x320 with Pillow and writes RGB565 to `/dev/fb0`, only when the frame bytes change.

| State | Display |
|---|---|
| no data, or `stale` | static gray screen: "NO LOGGER DATA" / "NO LIVE DATA" |
| `dpf_regen_active` set | near-black background, "ACTIVE REGENERATION / DO NOT SWITCH OFF / UNTIL IT FINISHES", soot, EGT before the DPF |
| otherwise | 2-column grid (soot, RPM, speed, coolant, diff pressure, catalyst temp, intercooler, MAF, since regen, regen state, odometer) |

Background colour: green below 14 g, amber 14 to 17 g, then a linear blend from amber to dark red (`#8b0000`) reaching full dark red at 28 g (`SOOT_GREEN_MAX`, `SOOT_AMBER_MAX`, `SOOT_DANGER_MAX` — the owner's own choices, not a specification). `python3 screen/soot_panel.py --demo` cycles fixed states every 30 s. The panel needs `requests` and `Pillow`, which are **not** in `requirements.txt`, and the DejaVu fonts (`fonts-dejavu-core`). See also [Screen renderer: the Pi's `soot_panel.py`](#screen-renderer-the-pis-soot_panelpy) below.

For the data-flow picture and column list of this repository's own ESP32-S3 firmware, see [architecture.md](architecture.md).

---

## PID map: the Pi's `pi:` citations

*(moved from `docs/pid-map.md`)*

The verified-table rows in [pid-map.md](pid-map.md) cite `pi: <line numbers>` for most columns — those line numbers refer to `obd/pid_registry.py` in the separate `obd-pi` repository. They cannot be opened or checked by a reader of this repository; only the `s3:`/`s3dec:` citations (this repo's own `src/obd/pid_registry.h` / `pid_decode.cpp`) are independently verifiable here. The Pi and S3 decoders were derived together from the same Car Scanner reference session, which is why the citations exist side by side in that table.

At the time of that reference session (2026-09-13), the derivation tooling
used was the Pi project's own copy of `carscanner_parse.py`/`carscanner_solve.py`:
it split the raw log on `>` prompts, matched each echoed request to a name in
a hard-coded `REQS` table, reassembled the ISO-TP payloads (accepting reply
ids `7E8` and `7DC`), then aligned the two series using RPM (a standard
formula, u16 x 0.25) and tried every byte offset and width (u8, i8, u16,
i16, u24, u32) of every request at sample shifts of -2 to +2, keeping
candidates with correlation above 0.995 and fitting `value = a*x + b`
(columns with fewer than 1000 samples or zero variance were skipped). Generic,
car-agnostic versions of the same two scripts are now published in **this**
repository's own `tools/` directory — see the update note in
[Dev setup, tests, and the Astra discovery tooling](#dev-setup-tests-and-the-astra-discovery-tooling-from-contributingmd)
below.

Battery voltage (`0142`) was added on the S3 only and, as of the notes this page was carried over from, was not yet in the Pi registry.

---

## Telegram alerts (Pi)

*(moved from `docs/telegram-alerts.md`)*

The Pi logger also sends regen START / END / INTERRUPTED alerts (`alerts/`, in the `obd-pi` repo). Put `TG_BOT_TOKEN` and `TG_CHAT_ID` in the Pi's untracked `config_local.py` (never in git), restart `obd-logger`, and test with `python3 -m alerts.telegram_alerts test` from the `obd-pi` repo root. Use a separate bot per device — do not reuse one bot token across the Pi and an ESP32-S3 board. The Pi sends no daily report or status pings (that is S3-only, documented in [telegram-alerts.md](telegram-alerts.md)).

---

## Dev setup, tests, and the Astra discovery tooling (from `contributing.md`)

*(moved from `docs/contributing.md`)*

**Python parts (Pi logger, dashboard, tools)** live in the separate, private `obd-pi` repo, not here:

```bash
python3 -m venv venv
venv/bin/pip install -r requirements.txt            # pyserial
venv/bin/pip install numpy                          # only for tools/carscanner_solve.py
venv/bin/pip install requests pillow                # only for screen/soot_panel.py
```

Run from the `obd-pi` repo root (modules import `config` and `obd.*` relative to it): `python3 main.py` needs an adapter; `python3 webui/server.py` runs against any directory of `session_N.csv` files (set `LOG_DIR` in `config.py`). The soot panel looks for DejaVu fonts in `/usr/share/fonts/truetype/dejavu`; without them it falls back to a tiny bitmap font that looks broken. On a desktop, import `render()` from `screen/soot_panel.py` and save the returned image instead of writing `/dev/fb0`.

**Pi-side test example** (this repository has no Python modules named `obd.isotp` or `obd.pid_registry` — its equivalent is `src/obd/isotp.cpp` / `isotpExtract` and `src/obd/pid_decode.cpp` / `dec_rpm`, in C++):

```python
# obd-pi repo only — run from that repo's root
from obd.isotp import extract_payload
from obd.pid_registry import dec_rpm

def test_single_frame_rpm():
    payload = extract_payload("7E804410C1AF8", "7E8")   # 7E8 04 41 0C 1A F8
    assert payload.hex() == "410c1af8"
    assert dec_rpm(payload) == 1726.0

def test_wrong_id_is_ignored():
    assert extract_payload("7E804410C1AF8", "7DC") is None
```

**Update, 2026-10-07:** an earlier version of this page (and of
[contributing.md](contributing.md) and [adapting/other-cars.md](adapting/other-cars.md))
said the discovery and Car-Scanner-alignment tooling (`discover.py`,
`carscanner_parse.py`, `carscanner_solve.py`) did not exist in either
published repo, and that an earlier, Astra-specific discovery script
(`astra/discovery_pi/discover.py`) existed only in the maintainer's local,
unpublished working copy. That gap has since been closed: generic,
car-agnostic versions of all three scripts are now published in **this**
repository's own [`tools/`](../tools/) directory (see
[`tools/README.md`](../tools/README.md)), and the current
[contributing.md](contributing.md#adding-a-pid-the-car-scanner-method) and
[adapting/other-cars.md](adapting/other-cars.md) point at them directly.
This paragraph is left here only as a record of that history.

**Keeping registries in sync (maintainer-only, cross-repo):** `obd/pid_registry.py` in the `obd-pi` repo (private) takes a `dec_*` function and a `PidDef(name, unit, header, request_hex, frames, prefix, decode)` row, offsets counted from the service byte. This is a step only the maintainer of both repos can do; an outside contributor to this public repo cannot edit a private repo they don't have.

---

## Testing: the Pi project's pytest suite

*(moved from `docs/tech/testing.md`)*

This repository (`obd-esp32`) has **no automated tests of any kind** today — no `test`-named files and no CI workflow anywhere in this tree. The suite described below belongs to the separate Pi project instead; it was, at the time these notes were written, staged at `_repo_prep/tests_draft/` (a pre-split staging area outside both published repos) and intended to become `tests/` in the `obd-pi` repo.

[ran, 2026-09-23] Copied `tests_draft/` to a scratch `tests/` beside symlinks to the Pi repo root and its `tools/`, fresh venv with `pytest pillow requests` (no pyserial): `221 passed, 1 xfailed` in about 2.5 s.

Running: `python3 -m venv venv && venv/bin/pip install pytest pillow requests` then `venv/bin/pytest tests -q`. `tests/conftest.py` puts the repo root and `tools/` on `sys.path` and installs a stub `serial` module whose `Serial` raises `SerialException`, so pyserial and hardware are not needed. It provides a `frames` fixture that builds ELM ATH1/ATS0-style reply text from a payload.

Layout:

| File | Covers |
|---|---|
| `conftest.py` | path setup, `serial` stub, `frames` fixture |
| `payloads.py` | synthetic payload builders constructed from the documented byte offsets |
| `fake_elm.py` | scripted stand-in for `ElmClient` |
| `test_isotp.py` | single/multi-frame reassembly, echo lines, wrong ids, short frames |
| `test_pid_registry.py` | every decoder against hand-built payloads, prefix/length handling, table shape |
| `test_elm_client.py` | `send_command`, init sequence, header caching |
| `test_ble_transport.py` | name matching, characteristic picking, fake-sysfs adapter resolution |
| `test_main.py` | `poll_cycle`, the reconnect loop, `connect_adapter` |
| `test_session_store.py` | file numbering, CSV/raw/calibration output, one strict `xfail` for a `begin()`/`makedirs` bug |
| `test_webui.py` | record conversion, downsampling, idle flag, path-traversal regex, live/summary/schema builders |
| `test_alerts.py` | `RegenWatch` and `TelegramSender` |
| `test_soot_panel.py` | colour bands and gradient, image size/mode, static stale frames, RGB565 encoding |
| `test_dpf_and_tools.py` | idle-blockage rule, `carscanner_parse.reassemble` |
| `ci.yml` | a GitHub Actions draft, never executed (the project was not yet on a git host) |

Approximate test-function counts: alerts 11, ble 26, dpf/tools 4, elm 7, isotp 14, main 21, pid_registry 17, session_store 9, soot_panel 15, webui 10 (parametrisation expands these to 221 cases).

This repository's own firmware has none of this today. If this repo adds tests, see [release-and-versioning.md](tech/release-and-versioning.md) for the proposed PlatformIO/Unity build-gate checklist.

---

## Alerts internals (Pi)

*(moved from `docs/tech/alerts-internals.md`)*

Files: `alerts/regen_watch.py` (state machine), `alerts/telegram_alerts.py` (sender), glue in `main.py` (`feed_alerts`, `run_normal_loop`) — all in the separate Pi project. This repository's equivalent is `src/report/regen_watch.cpp` and `src/report/telegram_report.cpp`, duplicated per ESP32-S3 target (repo root and both `boards/*/firmware/src/report/`); its user-facing setup is documented in [telegram-alerts.md](telegram-alerts.md).

**Wiring:** `run_normal_loop` builds `sender = sender_from_config()` and `watch = RegenWatch()` once. `sender_from_config()` returns `None` unless both `TG_BOT_TOKEN` and `TG_CHAT_ID` are non-empty, in which case no alert code runs at all. Each cycle, after the CSV row is written and only if `sender` exists, `feed_alerts` is called with the values of `ALERT_COLUMNS`: `dpf_regen_active, egt_before_dpf_c, dpf_soot_level_g, dpf_dist_since_regen_mi, dpf_diff_pressure_hpa`.

`feed_alerts` wraps everything in `try/except Exception` and prints the traceback to stderr, so an alerting bug can never stop logging. It sends the event text only for `START`, `END`, `INTERRUPTED`; the `TICK` event type exists in `RegenWatch` (a "still running" ping every 30 s) but `main.py` does not send it.

Credentials: `TG_BOT_TOKEN`/`TG_CHAT_ID` belong in the untracked `config_local.py`. The token is embedded in the request URL inside `_post`; nothing logs the URL.

**RegenWatch state machine:** pure logic, no I/O; `update(now_ms, wall, active, egt, soot, dist, pressure) -> RegenEvent`. It is described as a port of the S3's `src/report/regen_watch.cpp`, whose constants match (`STREAK_ROWS=3`, `GAP_MS=120000`, `TICK_MS=30000`). States: idle or regen-open, with streak counters for active/inactive rows, an interrupted-after-gap check (`now_ms - last_valid_ms > GAP_MS`, i.e. data resumed after more than 2 minutes with a regen still open), and the three message types (`START`, `END`, `TICK`) described in [telegram-alerts.md](telegram-alerts.md) for this repository's own firmware. Timestamps in the Pi's alert text are only appended when `clock_synced()` (via `timedatectl show -p NTPSynchronized --value`, 3 s timeout, 60 s cache) says the system clock is NTP-synced.

**Telegram sender:** `TelegramSender(token, chat_id, post=None, max_queue=4, backoff=BACKOFF_S, sleep=None)`. `send(text)` appends to a `collections.deque(maxlen=4)`; when full, the oldest message is silently discarded. A daemon thread pops the head, posts it, and retries with backoff `(15, 30, 60, 120, 300)` seconds on a retryable failure (offline/DNS/5xx/429), or drops it on a permanent one (4xx). Manual check: `python3 -m alerts.telegram_alerts test` sends one real message.

11 pytest cases cover this (`tests_draft/test_alerts.py`): start/end streaks, peak EGT tracking, interrupted-after-gap, missing-flag handling, clock-shown-only-when-synced, queue order, offline retry without loss, newest-4 retention, permanent-rejection drop, and `feed_alerts` error-swallowing. See [Testing](#testing-the-pi-projects-pytest-suite) above.

---

## Data formats (Pi: CSV, raw log, and HTTP API)

*(moved from `docs/tech/data-formats.md`)*

Everything in this section is the Pi logger's output (`storage/session_store.py`, `webui/server.py`), from the separate Pi project. This repository's ESP32-S3 targets have a different column set, order and HTTP API shape — see [architecture.md](architecture.md) and [adapting/other-apps.md](adapting/other-apps.md) for the real, verified ESP32-S3 shapes, including the fact that `/api/live` on this repo's firmware has no `has_data`/`stale`/`latest` wrapper at all (confirmed against `src/web/web_ui.cpp`).

`LOG_DIR` defaults to `/home/ix35/obd-logs` (`config.py`).

| File | Written by | Notes |
|---|---|---|
| `session_N.csv` | `SessionStore.write_row` | `N` = first index for which the file does not yet exist |
| `raw_N.log` | `SessionStore.write_raw` | Same `N`; opened in append mode on first use |
| `calibration.csv` | `write_calibration_row` | Only with `CALIBRATION_MODE = True`; shared across runs |

`session_N.csv` header: `unix_time` followed by 21 `COLUMNS` from `pid_registry.py` (same list as in the [Architecture](#architecture-pi-logger-services-dashboard-soot-panel) section above). Numbers use `f"{v:.3f}"`; `None`/NaN is an empty cell. Each row is flushed and `os.fsync`ed. A known defect (per a strict `xfail` in the Pi test suite): `SessionStore.begin()` calls `os.makedirs` outside its `try`, so an unusable `LOG_DIR` raises instead of returning `False`.

`raw_N.log`: tab-separated, `unix_time<TAB>header<TAB>request+digit<TAB>reply`. Thinning: while any value decodes, every cycle's raw lines are written; during a dead stretch the first blank cycle and every 60th after it are kept.

`calibration.csv`: `unix_time,label,raw_response`, labels from `CALIBRATION_REQUESTS` (`21948001`, `2103`, `211B`, `2101_eps`).

**HTTP API (`webui/server.py`):** stdlib `ThreadingHTTPServer` on `0.0.0.0:8080`. Read-only, no authentication, `Cache-Control: no-store`.

| Path | Response |
|---|---|
| `/`, `/index.html`, `/app.js`, `/style.css` | files from `webui/static/` |
| `/api/live` | see below |
| `/api/schema` | `{"pids": [...], "dpf_idle_pressure_threshold_hpa", "dpf_idle_rpm_ceiling", "dpf_temp_warn_threshold_c"}` |
| `/api/sessions` | array of session summaries, newest by mtime first |
| `/api/sessions/session_N.csv` | `{name, records, summary}`, downsampled to at most 400 points |

`/api/live` shape when data exists:

```json
{
  "has_data": true,
  "session": "session_N.csv",
  "stale": false,
  "age_s": 1.2,
  "latest": {"unix_time": 0.0, "engine_rpm": 0.0, "...": null},
  "dpf_idle_flag": false,
  "records": [ {"...": 0.0} ]
}
```

`records` is the last 180 rows; `stale` is true when `age_s` is null or greater than 8 s; `dpf_idle_flag` reuses the same idle-blockage rule described in the Architecture section above. The frontend tints the "EGT Before DPF" tile amber using `dpf_temp_warn_threshold_c` (300 by default) from `/api/schema`, even though a code comment in `config.py` says that threshold is meant for `dpf_zone_temp_c` — a known mismatch in the Pi project's own code.

---

## Protocol and PIDs: the Pi's Python implementation

*(moved from `docs/tech/protocol-and-pids.md`)*

This describes the separate Pi logger project's Python implementation (`obd/pid_registry.py`, `obd/elm_client.py`, `obd/isotp.py`) — not this repository. This repository's own equivalent, duplicated per ESP32-S3 target, is `src/obd/pid_registry.h` + `pid_decode.cpp` + `elm_client.cpp` + `isotp.cpp` at the repo root and under each `boards/*/firmware/src/obd/`. The verified column table that applies to both is in [pid-map.md](pid-map.md).

**Layers:** `PID_TABLE` row → `ElmClient.set_header()` (`ATSH<hdr>`) → `ElmClient.query_raw()` (`"<request><digit>\r"`) → `extract_payload()` ISO-TP reassembly → prefix check → `decode()` (fixed offsets + scale).

**ELM327 conversation:** `send_command` clears the input buffer, writes `cmd + "\r"`, then polls non-blocking reads until a chunk contains `>` or a deadline passes (2.0 s default, 3.0 s for `query_raw`). It never raises on timeout. Init sequence: `ATZ`, `ATE0`, `ATL0`, `ATH1`, `ATS0`, `ATSP6` (ISO 15765-4, CAN 11-bit, 500 kbit/s), `ATST96` (600 ms reply timeout). `ATSP6` was chosen because auto-search took 8-10 s on this car, longer than the logger's own timeout. (This repository's firmware uses `ATST32` / 200 ms instead — see `src/obd/elm_client.cpp`.)

**Request format:** `query_raw(request_hex, frames)` sends `request_hex + frames`, the ELM327 "expected number of response lines" digit (`B` = 11). A clone adapter that does not understand the digit is unverified on the Pi; this repository's firmware retries without it on `?` (see [pid-map.md](pid-map.md)).

**Headers and reply IDs** (`RX_ID` in `obd/pid_registry.py`): `7DF`→`7E8` (functional, mode 01), `7E0`→`7E8` (ECM, mode 21), `7D4`→`7DC` (steering/EPS).

**ISO-TP reassembly (`obd/isotp.py`):** splits text on `\r`/`\n`, keeps lines that are long enough, pure hex, and prefixed with the expected reply id; a `0` PCI nibble is a single frame, `1` is a first frame (12-bit total length, followed by `2`-nibble consecutive frames). Consecutive-frame sequence numbers are **not checked**, and duplicates/out-of-order frames are not detected. The logger never sends an ISO-TP flow-control frame; it relies on the adapter doing that.

**Decoder conventions:** offsets are from the start of the payload (byte 0 = service echo, byte 1 = PID/local id); helper functions return `None` on a too-short payload rather than raising.

**PID table and "measured vs inferred" summary:** the full row-by-row decode table (header, request+digit, prefix, decode formula, M/S/F status) and its measured/inferred basis are the same facts already captured, with citations, in [pid-map.md](pid-map.md)'s verified table — this page's own copy of that table has not been kept in sync and should not be treated as a second source of truth.

**Reproducing the derivation:** `tools/carscanner_parse.py` and `tools/carscanner_solve.py` (both `obd-pi` repo only) — see [Dev setup, tests, and the Astra discovery tooling](#dev-setup-tests-and-the-astra-discovery-tooling-from-contributingmd) above.

---

## Transports: rfcomm, BLE, USB dongle (Pi)

*(moved from `docs/tech/transports.md`)*

Selected by `TRANSPORT` in `config.py` ("rfcomm" default, or "ble"), in the separate Pi project. Per-device overrides go in an untracked `config_local.py`. This repository's ESP32-S3 targets only support BLE (`src/obd/elm_client.cpp` per target); there is no rfcomm/Bluetooth-Classic path in this repository.

**rfcomm (Bluetooth Classic SPP, used with the OBDLink MX+):** `scripts/bind_rfcomm.sh` runs `rfcomm release 0` then `rfcomm bind 0 <MAC> 1` (channel 1 assumed to be SPP), waits 1 s and checks `/dev/rfcomm0` exists. The MAC is hard-coded in the script and in `OBD_BT_MAC` in `config.py` (the config value is unused by the code — the script keeps its own copy). `obd-logger.service` runs the bind as `ExecStartPre=-...` (leading `-` ignores failure) and runs as root because `rfcomm bind` needs it.

**BLE (`obd/ble_transport.py`):** for BLE ELM327 adapters (Konnwei, vLinker, Veepeak BLE...), using `bleak` on top of BlueZ, imported lazily only when `TRANSPORT == "ble"`. `BleTransport` runs a private asyncio loop in a daemon thread and exposes blocking calls. Discovery: if `BLE_ADDRESS` is set, connect by address; otherwise scan and pick the strongest-RSSI device whose name matches a hint list (`OBD, ELM, VLINK, VEEPEAK, VGATE, KONNWEI, OBDLINK, MX+, ...`). Characteristic choice prefers the Nordic UART service, then any non-generic service with separate notify/write characteristics. Bring-up helpers: `python3 -m obd.ble_transport scan` / `dump ADDRESS`.

**USB dongle:** `resolve_adapter("usb")` scans `/sys/class/bluetooth/hci*` for the first adapter whose `device` symlink resolves through `/usb`, so the BLE adapter stays on a separate controller from the MX+'s onboard one (hciN numbering can change between boots). Verified offline only (faked sysfs in unit tests), not on a car.

**One central at a time:** a BLE ELM327 adapter accepts a single connection — this is a general BLE-adapter property that also applies to this repository's ESP32-S3 firmware, not just the Pi (see [transports.md](tech/transports.md) and [adapting/other-cars.md](adapting/other-cars.md)). If the ESP32-S3 (or a phone app) is connected, the Pi cannot connect, and vice versa; run one logger at a time.

**Failure handling:** `run_normal_loop` catches every exception, writes one blank row, closes the adapter and retries every 5 s. After `BLANK_STREAK_LIMIT = 10` consecutive fully blank cycles it forces a reconnect and re-init.

---

## Dependency licensing: the Pi project's Python dependencies

*(moved from `docs/licensing/dependency-compatibility.md`)*

The Pi project (`obd-pi` repo, `requirements.txt`) has its own dependency list, separate from this repository's three PlatformIO targets:

| Component | Where used | Licence (as believed) | With GPL-3.0-or-later |
|---|---|---|---|
| pyserial >=3.5 | `obd-pi` serial transport | BSD-3-Clause | Compatible |
| Pillow (`pillow`) | `screen/soot_panel.py` drawing | HPND (MIT-CMU style) | Compatible |
| requests | `alerts/telegram_alerts.py`, soot panel | Apache-2.0 | Compatible |
| bleak | `obd/ble_transport.py` (BLE only) | MIT | Compatible |
| numpy | `tools/carscanner_solve.py` only (dev tool, not in requirements.txt) | BSD-3-Clause (bundled parts vary) | Compatible |
| DejaVu fonts | `screen/soot_panel.py` loads from `/usr/share/fonts/truetype/dejavu` at runtime; not bundled | Bitstream Vera licence + public-domain additions | Fine (permissive) |
| Web UI (`webui/static`) | Dashboard | No external libraries/CDN URLs found by grep | n/a |

None of these cells were re-verified in this repository's own review pass (they were carried over unverified). This repository's own dependency table — NimBLE-Arduino, Adafruit GFX, GFX Library for Arduino (moononournation), the Arduino-ESP32 core, and the Telegram Bot API/root CA used by the firmware — is the actual, current table in [dependency-compatibility.md](licensing/dependency-compatibility.md).

---

## Workstation setup: imaging and SSH-ing into the Pi

*(moved from `docs/workstation-setup.md`)*

These steps are for the separate, still-private `obd-pi` project's hardware (a Raspberry Pi 4 and a microSD card), not for anything in this repository. They are kept here only for someone who happens to be setting up both projects at once. Replace `<pi-user>`, `<pi-host>` and `<pi-ip>` with your own values; never commit private keys or Wi-Fi passwords.

**Imaging the SD card (any OS):** download Raspberry Pi Imager from `https://www.raspberrypi.com/software/` (on Linux, `sudo apt install rpi-imager` on Raspberry Pi OS, or the AppImage elsewhere). In the customisation screens set a hostname (reachable as `<hostname>.local`), a username and password, Wi-Fi details, and the SSH option with public-key authentication — paste your public key there rather than setting a password up manually afterwards. The dialog is documented as expecting an RSA public key file; whether it accepts Ed25519 is unverified, so generating an RSA key is the safer option if you plan to use this step to install your key.

**SSH key setup, per OS:**

- *Windows:* OpenSSH ships as an optional Windows 10+ feature (`Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*'`, then `Add-WindowsCapability`). `ssh-keygen -t ecdsa` creates `id_ecdsa`/`id_ecdsa.pub` under `C:\Users\<you>\.ssh\`. Windows has no `ssh-copy-id`; pasting the public key into Imager's SSH step is the simplest route. WSL 2 can also flash/SSH, but USB serial devices need `usbipd-win` to be visible there.
- *Linux:* `ssh-keygen` then `ssh-copy-id <pi-user>@<pi-ip>` then `ssh <pi-user>@<pi-ip>` (default key `~/.ssh/id_rsa`). If `ssh-copy-id` is missing: on the Pi `mkdir .ssh && chmod 700 .ssh`, from your machine `scp .ssh/id_rsa.pub <pi-user>@<pi-ip>:.ssh/authorized_keys`, then on the Pi `chmod 644 .ssh/authorized_keys` (this overwrites any existing file). `<pi-host>.local` needs mDNS on your workstation; otherwise use the Pi's IP from your router.
- *macOS:* ships an OpenSSH client; use the same commands as Linux. `ssh-copy-id` may be absent on some versions — use the manual `scp` method above if so. `.local` resolves via macOS's built-in mDNS.

**Common errors specific to the Pi path:** a Bluetooth-adapter question is not a workstation matter — see the Pi project's own `pi-setup.md` (not published).

This repository's own ESP32-S3 flashing steps (PlatformIO install, serial permissions, `pio run -t upload`) are the real, current content of [workstation-setup.md](workstation-setup.md) — only the imaging/SSH material above has moved here.

---

## Garage install: the Pi 4 variant

*(moved from `docs/garage/02-install-in-10-minutes.md`)*

A garage that also has a Pi 4 build (Bluetooth adapter, separate still-private project) can follow the same physical steps as the ESP32-S3 install, with these Pi-specific differences: a new `session_N.csv` appears in the Pi's log directory (rather than the S3's web UI showing live rows) to confirm the logger sees the adapter; the clock comes from the Pi's own system clock rather than NTP/phone; log files are copied from the Pi's configured log directory rather than an SD card; and a Raspberry Pi 4 is a full computer, so it draws far more power than an ESP32-S3 and should not be left in a hot, unventilated spot such as a sun-baked dash (speculation, not tested).

This repository's own ESP32-S3 install steps are the real, current content of [garage/02-install-in-10-minutes.md](garage/02-install-in-10-minutes.md).

---

## AI setup prompt: the Pi-specific interview branch

*(moved from `docs/ai-setup-prompt.md`)*

Both AI-assistant prompts in [ai-setup-prompt.md](ai-setup-prompt.md) ask the assistant to find out, as part of its hardware interview, whether the person also has a Raspberry Pi 4 build. The Pi-specific technical detail behind that question (relevant only if someone has both projects checked out): the Pi supports Bluetooth Classic SPP by default and, via a USB BLE dongle, BLE as well — set `TRANSPORT = "ble"` in the Pi's `config_local.py`. Only one central can be connected to a BLE OBD adapter at a time, so close any phone app first. The BLE path on the Pi has been verified offline only (see [Transports](#transports-rfcomm-ble-usb-dongle-pi) above), never on a car.

---

## Screen renderer: the Pi's `soot_panel.py`

*(moved from `docs/tech/screen-renderer.md`, which now documents this repository's own ESP32-S3 touchscreen UIs instead — see [screen-renderer.md](tech/screen-renderer.md))*

A full-screen status display for a 3.5" SPI panel on the Pi, from the separate, still-private Pi project. `screen/soot_panel.py` and the hardware/overlay setup story for it are not part of this repository.

**Data flow:** polls `http://localhost:8080/api/live` once a second (`requests`), decides a display state, renders an image with Pillow, converts to RGB565, and writes raw bytes to the framebuffer device `/dev/fb0` — but only when the frame actually changed, to avoid needless writes.

**States:**

| Condition | Display |
|---|---|
| no session data at all | static grey screen, "NO LOGGER DATA" |
| data exists but `stale` (no row in the last 8 s, plus the panel's own extra ~15 s cutoff) | static grey screen, "NO LIVE DATA" |
| `dpf_regen_active` is set | near-black background; "ACTIVE REGENERATION / DO NOT SWITCH OFF / UNTIL IT FINISHES"; soot and EGT-before-DPF shown large |
| otherwise (normal) | a 2-column grid: soot, RPM, speed, coolant, differential pressure, catalyst temp, intercooler, MAF, distance since regen, regen state, odometer |

**Colour coding:** background colour is a function of the soot reading in grams — green below 14 g, amber from 14 to 17 g, then a linear blend from amber to dark red (`#8b0000`) that reaches full dark red at 28 g. These three constants (`SOOT_GREEN_MAX`, `SOOT_AMBER_MAX`, `SOOT_DANGER_MAX`) are the project owner's own choices for one car, not a manufacturer specification — see [disclaimer.md](disclaimer.md).

**Other notes:** `python3 screen/soot_panel.py --demo` cycles the fixed states every 30 s for bench testing without a car. The panel needs `requests` and `Pillow` (not listed in the Pi project's `requirements.txt` at the time these notes were written) and the DejaVu fonts package (`fonts-dejavu-core`) — without them it falls back to a tiny bitmap font that looks broken. This repository's own ESP32-S3 boards use completely different display libraries and font rendering; see [screen-renderer.md](tech/screen-renderer.md).

---

## Also mentioned elsewhere, kept short on purpose

A few other pages mention the Pi project in a single sentence and were left as-is rather than expanded here, because they already follow the "short pointer, not detail" pattern:

- [README.md](../README.md) and [docs/user-guide/01-what-it-does.md](user-guide/01-what-it-does.md) — one line each noting the Pi 4 version is a separate, not-yet-public project.
- [docs/esp32-c3-c6.md](esp32-c3-c6.md) and [docs/garage/06-business-models.md](garage/06-business-models.md) — one row/sentence noting the Pi 4 build exists and has been tested for longest on the real car.
- [docs/manual-setup.md](manual-setup.md) — "Target 4" is a short, clearly-labelled pointer to the Pi project's own `pi-setup.md`, not a description of its internals.
- [docs/adapting/other-apps.md](adapting/other-apps.md) and [docs/licensing/options.md](licensing/options.md) — compare the Pi's dashboard/`webui` briefly, where needed, against this repository's own ESP32 equivalent.
