# Using the logger with other apps and tools

Labels: **[exists]** is in this repository today; **[proposed]** is a design idea only; **[external]** is a third-party feature described from general knowledge and **not tested with this project**.

## What the logger actually produces today

| Output | Where | Status |
|---|---|---|
| `session_N.csv`: one row per poll (about 1 Hz), header row lists columns, columns named like `engine_rpm`, `dpf_soot_level_g`, `dpf_diff_pressure_hpa`, with `unix_time` when the clock is known | Pi: `LOG_DIR` (`config.py`); ESP32: SD card (`/obd`) | **[exists]** |
| `raw_N.log`: tab-separated raw adapter traffic (time, header, request, reply) | Pi: next to the CSV | **[exists]** |
| Web dashboard and JSON: `/`, `/api/live`, `/api/sessions`, `/api/sessions/<name>`, `/api/schema` (Pi, port `DASHBOARD_PORT`, default 8080, no authentication, HTTP) | Pi | **[exists]**; `/api/schema` currently raises (see contributing.md known issues) |
| ESP32 web UI: `/api/live`, `/api/files`, `/api/time`, `/api/send`, `/api/report`, `/api/testalert`, `/api/statusnow` | ESP32 | **[exists]** (endpoint shapes not re-documented here) |
| Telegram messages for regen start/end and reports | Pi and ESP32 | **[exists]** |
| MQTT, InfluxDB, Prometheus, or any push export | none | **not present** (searched the source: no references) |

`/api/live` returns `has_data`, `session`, `stale`, `age_s`, `latest` (the newest row as a name-to-value record), `dpf_idle_flag` and the recent `records`. This is the easiest hook for anything below.

Security note: the dashboard has no login and speaks plain HTTP. Do not expose it to the internet. Put it behind your own network controls or a VPN. It contains your odometer and timestamps.

## Car Scanner (phone app, ELM327)

- **Role today [exists]**: a *reference*. The decoders were derived from a Car Scanner session, and the repository parses its raw log and CSV export (`tools/carscanner_parse.py`, `carscanner_solve.py`). Not a data consumer.
- **Live alongside the logger**: works only if the adapter allows two connections. The OBDLink MX+ can pair with a phone and the Pi at once in some modes; BLE adapters usually accept one central only (see the Pi BLE dongle note in the project docs). Otherwise run them at different times.
- **Export back into the project [exists, partial]**: CSV and raw log import is for decoding, not for the dashboard. **[proposed]** an importer that turns a Car Scanner CSV into a `session_N.csv` so past drives show in the dashboard.
- **Feeding Car Scanner from the logger**: not possible; it talks to the adapter directly.

## Torque (Android, ELM327)

- **[external]** Torque supports custom PIDs by CSV and can log to files or upload to a web endpoint. Torque's log format and upload API are documented by its author.
- **Custom PID import [proposed]**: generate a Torque custom-PID CSV from a profile file (see [pid-profile-format.md](pid-profile-format.md)), so the verified ix35 DPF PIDs are usable in Torque. Torque's PID CSV lists request, formula in its own expression syntax, min/max and unit; the mapping from `scale`/`add`/`offset` would be mechanical. Needs the extended (manufacturer) header setting; verify that Torque can set `ATSH7E0` and the frame-count digit before trusting it.
- **Data upload from Torque to a local web server [proposed]**: a small endpoint accepting Torque's HTTP upload and writing rows.

## OBD Fusion (iOS/Android, ELM327)

- **[external]** supports custom PIDs (enhanced PID pack files) and logging.
- **[proposed]** export a PID pack from the profile file, as for Torque. Not attempted; format details need checking against OBD Fusion's documentation before writing an exporter.
- Same one-connection limit on the adapter.

## Home Assistant

- **Nothing exists.** There is no Home Assistant integration.
- **Poll the dashboard as a REST sensor [proposed, easy]**: Home Assistant's `rest` / `rest_sensor` can read `http://<pi>:8080/api/live` and extract `latest.dpf_soot_level_g` with a value template. Works with today's endpoint and no change to this project. Untested. `stale` and `age_s` let you show "unknown" when the car is off.

  ```yaml
  # untested sketch, Home Assistant configuration.yaml
  rest:
    - resource: http://PI_ADDRESS:8080/api/live
      scan_interval: 30
      sensor:
        - name: "DPF soot"
          value_template: "{{ value_json.latest.dpf_soot_level_g if value_json.has_data else none }}"
          unit_of_measurement: "g"
  ```
- **MQTT [proposed]**: a small publisher (in the `obd-pi` repo, using `paho-mqtt`) sending each new row to topics such as `obd/<vehicle>/dpf_soot_level_g` with retained state, plus Home Assistant MQTT discovery messages. The car is only reachable when it is on the home network, so retain the last value and publish an availability topic (`online`/`offline`). Also consider the alerts: regen start/finish is already detected in `alerts/regen_watch.py` and could publish events instead of, or as well as, Telegram.
- **ESP32**: could publish to MQTT directly over Wi-Fi in the same way. Needs a library (for example PubSubClient or AsyncMqttClient) and RAM budget check on the S3. **[proposed]**.
- **Telegram** remains the existing alert path **[exists]**.

## Grafana (with InfluxDB, Prometheus, or a CSV/JSON source)

- **Nothing exists.** No exporter is present.
- **[proposed] Path A: CSV to database.** A script tails `session_N.csv` and writes rows to InfluxDB (line protocol) or another time-series DB, using `unix_time` as the timestamp (rows without a clock cannot be plotted in absolute time; sessions of the ESP32 use `millis` until a time sync). Grafana then charts soot, pressure and temperatures over days and weeks, which is the project's stated purpose ("trends over days and weeks").
- **[proposed] Path B: Prometheus.** A `/metrics` endpoint. Less natural, since a car is not always online and Prometheus scrapes on a schedule.
- **[proposed] Path C: Grafana Infinity/JSON plugin** reading `/api/live` and `/api/sessions/<name>` directly. **[external]** the Infinity data source is a third-party plugin; not tested.
- Sensible panels: soot (g) against distance since last regen, differential pressure at hot idle only (needs a coolant filter, the idle flag in code does not check engine temperature), regen events as annotations.

## Other phone and desktop tools

| Tool | What is possible | Status |
|---|---|---|
| Any browser on the same network | Use the dashboard at `http://<pi-ip>:8080/` | **[exists]** |
| Spreadsheet (Excel, LibreOffice, Google Sheets) | Open `session_N.csv` directly; header names are self-describing | **[exists]** |
| Python/pandas/Jupyter | `pd.read_csv("session_N.csv")` and plot | **[exists]** (works on the CSV as-is) |
| Telegram | regen and report messages | **[exists]** |
| Torque / OBD Fusion / Car Scanner | see above | mostly **[proposed]** |
| ABRP (A Better Routeplanner) and similar | EV-oriented; not relevant to a diesel DPF | not applicable |
| Web push / phone widget | any tool that can fetch `/api/live` | **[proposed]** |

## Suggested order if someone builds this

1. Home Assistant `rest` sensor: zero code (proposed above).
2. Fix `/api/schema`, document `/api/live`, add optional token auth **[proposed]**.
3. CSV importer/exporter.
4. MQTT publisher (Pi), then Grafana via Influx.
5. Torque/OBD Fusion PID pack export from the profile format.

## Caveats

- The data is a hobby logger's view of ECU values on a car verified only for the ix35. Soot is the ECU's estimate. See [../disclaimer.md](../disclaimer.md).
- Anything that sends data off the local network (cloud dashboards, MQTT brokers on the internet) sends odometer readings and drive times. Decide that deliberately.
- Do not put Telegram tokens, Wi-Fi passwords, MQTT credentials or the adapter's Bluetooth address in shared configuration or screenshots.
