# Data formats

Everything here is the Pi logger's output (`pi/storage/session_store.py`, `pi/webui/server.py`). The ESP32-S3 has a different column set and order; see [../architecture.md](../architecture.md). Labels: [code] read from source, [unverified] not checked.

## Files in `LOG_DIR`

`LOG_DIR` defaults to `/home/ix35/obd-logs` (`pi/config.py`).

| File | Written by | Notes |
|---|---|---|
| `session_N.csv` | `SessionStore.write_row` | `N` = first index for which the file does not yet exist (`begin()` counts up from 0). One file per process run. |
| `raw_N.log` | `SessionStore.write_raw` | Same `N`. Opened in append mode on first use, so a run that never gets a reply leaves no file. |
| `calibration.csv` | `write_calibration_row` | Only with `CALIBRATION_MODE = True`; shared across runs (append). |

### session_N.csv

[code] Header row: `unix_time` followed by `COLUMNS` (`pid_registry.py`), 21 columns in this order:

```
unix_time, engine_rpm, coolant_temp, vehicle_speed, maf_flow, engine_load_pct,
egr_duty_pct, intake_air_temp_c, dpf_zone_temp_c, dpf_diff_pressure_hpa,
dpf_soot_level_g, intercooler_temp_c, egt_before_dpf_c, dpf_dist_since_regen_mi,
odometer_mi, dpf_regen_active, dpf_regen_burning, eps_speed_kmh, eps_steering_deg,
eps_voltage_v, dpf_odo_at_last_regen_mi
```

- Units per column are in `PID_TABLE` (`unit` field); they are not in the file. `dpf_regen_*` are 0/1.
- Numbers use `f"{v:.3f}"`; `None` or NaN is an empty cell. `unix_time` is `time.time()` at write time, three decimals.
- One row per poll cycle, so slightly slower than `POLL_INTERVAL_S` (1.0 s) plus the time to query. One row is also written (all blank) for every failed adapter attempt (see [transports.md](transports.md)).
- Each row is flushed and `os.fsync`ed.
- The header is written from the registry at file creation, so old files keep their own column list. Read by header name, not position; the Pi and S3 orders differ.
- `unix_time` comes from the Pi's system clock; with no RTC and no network in the car it can be wrong [code comment in `alerts/regen_watch.py`, `clock_synced`]. The dashboard's staleness test compares it with the current clock, so a wrong clock can show a live session as stale [inferred from `build_live`].

Known defect (see `tests_draft/test_session_store.py`, strict xfail): `SessionStore.begin()` calls `os.makedirs` outside its `try`, so an unusable `LOG_DIR` raises instead of returning `False`; the "logging disabled" warning in `main.py` is unreachable.

### raw_N.log

Tab-separated, one line per request: `unix_time<TAB>header<TAB>request+digit<TAB>reply`, where `unix_time` has three decimals and the reply's `\r`/`\n` are replaced by `|` [code: `write_raw`].

```
<unix_time>	7E0	2103B	7E8104B6103...|7E821...|7E822...
```

(Illustrative shape only.) Details:

- Reply is the adapter's raw text, unfiltered: could be `NO DATA`, `?`, `CAN ERROR`, or frames.
- A failed `ATSH` is logged as request `ATSH<hdr>`, reply `HEADER SWITCH FAILED`.
- Shared requests appear once per cycle (e.g. `2103B` once, although it feeds five columns).
- Thinning: while any value decodes, every cycle's raw lines are written. During a dead stretch (all cells blank) the first blank cycle and every 60th after it are kept (`blank_cycles_logged % 60 == 0` in `run_normal_loop`). The counter resets when data returns.
- One `fsync` per cycle.

### calibration.csv

`unix_time,label,raw_response`, with labels from `CALIBRATION_REQUESTS` (`21948001`, `2103`, `211B`, `2101_eps`).

## HTTP API (`pi/webui/server.py`)

Stdlib `ThreadingHTTPServer` on `0.0.0.0:DASHBOARD_PORT` (8080). Read-only; no authentication; responses have `Cache-Control: no-store`. Whole CSV files are parsed on every request (fine for hobby volumes) [code].

| Path | Response |
|---|---|
| `/`, `/index.html`, `/app.js`, `/style.css` | files from `pi/webui/static/` |
| `/api/live` | see below |
| `/api/schema` | see below |
| `/api/sessions` | array of session summaries, newest file (by mtime) first; empty sessions omitted |
| `/api/sessions/session_N.csv` | `{name, records, summary}`; records downsampled to at most 400 (`HISTORY_MAX_POINTS`, uniform stride). Name must match `^session_\d+\.csv$` (blocks path traversal); otherwise 404 |
| anything else | 404 |

Records are dictionaries keyed by CSV header: floats, or `null` for empty/non-numeric cells (`rows_to_records`).

### /api/live

Source: `build_live()`. Picks the newest `session_*.csv` by mtime.

- No session files: `{"has_data": false}`.
- File with header only: `{"has_data": false, "session": "session_N.csv"}`.
- Otherwise:

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

`records` is the last `LIVE_WINDOW_ROWS` = 180 rows. `age_s` is `time.time() - latest.unix_time` (null if that cell is empty); `stale` is true when `age_s` is null or greater than `STALE_AFTER_S` = 8 s. `dpf_idle_flag` = `rpm <= DPF_IDLE_RPM_CEILING and pressure >= DPF_IDLE_PRESSURE_THRESHOLD_HPA` on the latest record (`compute_dpf_flag`). The soot panel applies its own extra 15 s cutoff (see [screen-renderer.md](screen-renderer.md)).

### /api/schema

`build_schema()` returns `{"pids": [{"name","unit"} ...], "dpf_idle_pressure_threshold_hpa", "dpf_idle_rpm_ceiling", "dpf_temp_warn_threshold_c"}`.

Status: in the current source this works (the `p.confirmed` reference described in older notes, and in `../architecture.md` / `../contributing.md`, is gone; those drafts are stale) and `tests_draft/test_webui.py::test_build_schema_serialises` covers it. The frontend uses `dpf_temp_warn_threshold_c` (300 by default) to tint the "EGT Before DPF" tile amber (`app.js`, `LIVE_TILES` `warnKey`). Note the comment in `pi/config.py` says this threshold is for `dpf_zone_temp_c`; the UI applies it to `egt_before_dpf_c` [code mismatch].

### /api/sessions summary

`session_summary()`: `name, start_time, end_time, duration_s, row_count, max_rpm, max_coolant_temp, max_dpf_zone_temp_c, max_speed, soot_level_g (the max), dpf_idle_flag (any row flagged)`. Returns `null` (omitted) for a file with no data rows. Note `soot_level_g` is a maximum, not the final value.
