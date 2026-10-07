# Proposal: a per-vehicle PID profile file

> **This is a proposal. Nothing on this page exists in the code.** Today PIDs and decoders are hard-coded in `obd/pid_registry.py` (`obd-pi` repo) and, separately, in `src/obd/pid_registry.h` + `pid_decode.cpp` (`obd-esp32` repo), kept in sync by hand. `astra/design/reuse_plan.md` sketches a compile-time C++ profile; this page proposes a data-file alternative that both the Pi and (via a generator) the ESP32 could share. Feedback welcome before anybody builds it.

## Goals

1. One file per car (`profiles/ix35-d4fd.yaml`), no code change to add a car.
2. Same file drives the Pi at runtime, and generates the ESP32 table at build time (the ESP32 has little RAM and no need to parse YAML on-device).
3. Capture what the docs call "how it was verified" next to each PID, so unverified entries are visibly unverified.
4. Keep read-only: the schema has no way to express a write.
5. Cover only what the current decoders need: unsigned/signed ints at byte offsets with scale and offset, plus simple flag tests, plus derived columns.

## Non-goals

Full UDS/GMLAN scripting, security access, DTC databases, ECU flashing, anything that is not a plain read.

## Format choice

**YAML** for hand-editing with comments (the Pi could use PyYAML, a new dependency, or the profile could be loaded as JSON with the same structure to avoid one). **JSON** is the interchange form for the generator and the web UI. The two are equivalent; the example below is YAML.

## Example

```yaml
schema: 1
vehicle:
  id: ix35-d4fd-2014
  make: Hyundai
  model: ix35
  engine: "1.7 CRDi D4FD"
  year: 2014
  region: UK              # informs display units (miles)
  notes: "Only verified profile. Derived from a Car Scanner session on 2026-09-13."

bus:
  protocol: 6             # ATSP6, ISO 15765-4 CAN 11-bit 500k
  headers_on: true
  frame_count_digit: true # append expected-frames digit to requests
  frame_count_fallback: true

ecus:
  functional: { tx: "7DF", rx: "7E8" }
  ecm:        { tx: "7E0", rx: "7E8" }
  eps:        { tx: "7D4", rx: "7DC" }

units:
  distance: mile          # source is metres

requests:                 # one request may feed several columns
  rpm:        { ecu: functional, hex: "010C", frames: "1", prefix: "410C" }
  dpf_block:  { ecu: ecm,        hex: "2103", frames: "B", prefix: "6103", min_len: 75 }
  soot_block: { ecu: ecm,        hex: "219480", frames: "12", prefix: "61" }

columns:
  engine_rpm:
    request: rpm
    decode: { type: u16, offset: 2, scale: 0.25, add: 0 }
    unit: rpm
    poll: fast
    verified: { how: "carscanner-csv-fit", samples: 10447, date: 2026-09-13 }
  dpf_soot_level_g:
    request: soot_block
    decode: { type: u8, offset: 10, scale: 0.392157, add: 0 }   # 100/255
    unit: g
    poll: slow
    verified: { how: "carscanner-csv-fit", samples: null }
  dpf_regen_active:
    request: dpf_block
    decode: { type: flag, offset: 71, equals: 4 }
    unit: flag
    verified: { how: "one reference regen", validated: false }
  dpf_diff_pressure_hpa:
    request: { ecu: ecm, hex: "211B", frames: "1", prefix: "611B" }   # inline form
    decode: { type: u16, offset: 2, scale: 0.5 }
    unit: hPa

derived:
  dpf_odo_at_last_regen_mi:
    expr: "odometer_mi - dpf_dist_since_regen_mi"

probes:                   # requests that are tried but expected to fail; back off when they do
  - { name: intake_map_mbar, ecu: ecm, hex: "22280B", expect: unsupported }

features: { dpf: true, regen_watch: true, boost: false }

dashboard:
  tiles: [ { column: dpf_soot_level_g, label: "Soot", warn: 14, alert: 17 } ]
```

## Decode types

| `type` | Meaning | Extra fields |
|---|---|---|
| `u8`, `i8`, `u16`, `i16`, `u24`, `u32` | big-endian integer at `offset` from the service byte | `scale`, `add` |
| `flag` | 1 if the byte equals `equals` (or `mask` matches), else 0 | `equals`, `mask` |
| `bits` | extract a bit field | `bit`, `width` |
| `bitmap` | supported-PID bitmap (mode 01 standard) | base |
| `ascii` | text (VIN, calibration id) | offset, length |

Result: `value = raw * scale + add`. A column is blank if the reply is shorter than `offset + width` or the prefix does not match (matches today's behaviour: "if the payload is too short or does not start with the expected prefix, the cell is empty").

## Behaviours to keep

- Poll tiers and back-off for unsupported probes (ESP32 today: one attempt every 30 s after 5 failures).
- CSV header row listing every column, so old files stay readable.
- `verified` is metadata only; the dashboard could show an "unverified" badge (today `build_schema()` in `webui/server.py` tries this but is broken, see contributing.md known issues).
- A `VEHICLE` marker written into the raw log at start so sessions from different cars sharing storage stay distinguishable (idea from `astra/design/reuse_plan.md`).

## Open questions

1. **YAML dependency on the Pi** (PyYAML) versus JSON only. Recommend JSON at runtime, YAML as the authoring format converted by a script.
2. **Where do thresholds and regen logic live?** Regen alerts and the idle flag are code, not data. Options: keep them in code behind `features`, or add a tiny rule language. Recommend code for now.
3. **Generator for ESP32**: a script (`tools/gen_profile.py`) emitting a C header. The alternative (loading JSON from LittleFS at boot) costs RAM and a parser.
4. **Column name stability**: profile column names are the API for the dashboard and Telegram texts; a renamed column breaks them.
5. **Sharing profiles**: a `profiles/` directory in the repo, each with its own verification notes. Should unverified community profiles be accepted? Suggest a `status: verified | candidate` field and only ship verified ones by default.
6. **Multi-ECU requests and reply-id filtering** need the `ecus` table to support several accepted rx ids per request.
7. **Units**: keep the file in native units (metres) and convert by unit setting, or store the converted scale as the ix35 code does today (metres to miles).

## Migration path (proposed)

1. Write a script that dumps today's `PID_TABLE` into this format for the ix35 and check the round trip decodes identical values from `raw_N.log` files.
2. Add a profile loader on the Pi behind a config switch; keep `pid_registry.py` as fallback.
3. Generate the ESP32 header from the same file and diff against today's table.
4. Only then add a second car.
