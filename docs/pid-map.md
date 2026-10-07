# PID map (2014 Hyundai ix35 1.7 CRDi, D4FD)

This is the map from request to CSV column for one car. It will not give meaningful DPF data on any other vehicle. Every row below was read from the code, and the source is cited (`obd/pid_registry.py` in the `obd-pi` repo is abbreviated `pi:`, `src/obd/pid_registry.h` in the `obd-esp32` repo as `s3:`, `src/obd/pid_decode.cpp` as `s3dec:`).

> `pi:` citations point to line numbers in `obd/pid_registry.py` in the separate,
> still-private `obd-pi` repository (see the main README). They are kept because
> the Pi and S3 decoders were derived together from the same reference capture,
> but a reader of this public repo cannot open that file to check them — only
> the `s3:`/`s3dec:` citations point at files that exist in this repository.
> Background on the Pi project generally: [cross-project-notes.md](cross-project-notes.md).

## How to read the table

- **Request as sent** = request hex + the *frame-count digit*. ELM327 adapters accept one extra hex digit after a request meaning "expect this many response frames", so the adapter returns as soon as they arrive instead of waiting out the timeout. Car Scanner does the same on this car and the logger mirrors it. `B` is hex 11. Some clone adapters answer `?`; the S3 firmware then resends without the digit (`main.cpp`, `runRequest`); the Pi does not.
- **Header** is the CAN transmit id set with `ATSH` before the request. Replies come from `7E8` (for 7DF and 7E0) or `7DC` (for 7D4).
- **Payload offset** counts from the ISO-TP payload start: byte `[0]` is the service echo (`0x41` for mode 01, `0x61` for mode 21) and `[1]` is the PID / local id. `u16 [2]` means bytes 2 and 3 big-endian.
- Worked example: reply `7E8 04 41 0C 1A F8` has payload `41 0C 1A F8`; `u16 [2]` = `0x1AF8` = 6904; x 0.25 = 1726 rpm.

## Verified table

| CSV column | Header | Request as sent | Decode from payload | Unit | Source |
|---|---|---|---|---|---|
| `engine_rpm` | 7DF | `010C1` | u16 [2] x 0.25 | rpm | pi: lines 71, 105 |
| `coolant_temp` | 7DF | `01051` | u8 [2] - 40 | C | pi: 72, 106 |
| `vehicle_speed` | 7DF | `010D1` | u8 [2] | km/h | pi: 73, 107 |
| `maf_flow` | 7DF | `01101` | u16 [2] x 0.01 | g/s | pi: 74, 108 |
| `engine_load_pct` | 7DF | `01041` | u8 [2] x 100/255 | % | pi: 75, 109 |
| `egr_duty_pct` | 7DF | `012C1` | u8 [2] x 100/255 | % | pi: 76, 110 |
| `intake_air_temp_c` | 7DF | `010F1` | u8 [2] - 40 | C | pi: 77, 111 |
| `dpf_zone_temp_c` (see caveat) | 7DF | `013E1` | u16 [2] x 0.1 - 40 | C | pi: 78, 115 |
| `control_module_v` (battery), S3 only | 7DF | `01421` | u16 [2] x 0.001 | V | s3: 65; s3dec: 36 |
| `dpf_diff_pressure_hpa` | 7E0 | `211B1` | u16 [2] x 0.5 | hPa | pi: 82, 118 |
| `dpf_soot_level_g` | 7E0 | `219480012` | u8 [10] x 100/255 | g | pi: 84, 119 |
| `intercooler_temp_c` | 7E0 | `219480012` | u8 [6] - 50 | C | pi: 83, 120 |
| `egt_before_dpf_c` | 7E0 | `2103B` | u16 [48] x 0.0183966 + 99.013 | C | pi: 85, 121 |
| `dpf_dist_since_regen_mi` | 7E0 | `2103B` | u24 [56] x 0.000621371 (metres to miles) | mi | pi: 86, 122 |
| `odometer_mi` | 7E0 | `2103B` | u32 [59] x 0.000621371 (metres to miles) | mi | pi: 87, 123 |
| `dpf_regen_active` | 7E0 | `2103B` | 1 if byte [71] == 4, else 0 | flag | pi: 88-90, 124 |
| `dpf_regen_burning` | 7E0 | `2103B` | 1 if byte [19] == 21, else 0 | flag | pi: 91-93, 125 |
| `eps_speed_kmh` | 7D4 | `21013` | u8 [5] | km/h | pi: 97, 128 |
| `eps_steering_deg` | 7D4 | `21013` | i16 [11] x 0.1 (signed) | deg | pi: 98, 129 |
| `eps_voltage_v` | 7D4 | `21013` | u8 [3] x 0.1 | V | pi: 99, 130 |
| `dpf_odo_at_last_regen_mi` | derived | none | `odometer_mi - dpf_dist_since_regen_mi` (`main.py`, end of `poll_cycle`) | mi | pi: 134; `main.py` |

Rows sharing a request (`21948001`, `2103`) use one reply: the `2103` reply is at least 75 bytes long (the S3 decoders require 75), which is why it needs 11 frames. Expected payload prefixes: `41 <pid>` for mode 01; `61 1B` for `211B`; `61` only for `21948001`; `61 03` for `2103`; `61 01` for `2101` (Pi: the `prefix` argument in each `PidDef`). Units are miles because the car is a UK, mile-dashboard car and reports metres.

Resolution note: soot is one byte scaled by 100/255, so steps are about 0.39 g.

Every decoder takes the payload starting at the service byte. If the payload is too short, or does not start with the expected prefix, the cell is empty.

### Caveat: `dpf_zone_temp_c` is catalyst temperature

`013E` is the standard "catalyst temperature, bank 1 sensor 2" PID. The column kept its early name for continuity with existing logs and the dashboard, but it is **not** the DPF temperature. The temperature at the DPF inlet is `egt_before_dpf_c` (from `2103`). This is stated in a comment in both registries (`pi:` lines 78 and 112-114, `s3:` lines 70-71). The web UI tile and the panel label it "Catalyst Temp".

### Notes on individual rows

- `dpf_regen_active` is 1 from the start of a regen (warm-up) until the distance-since-regen counter resets (comment at `pi:` line 88). `dpf_regen_burning` is 1 during the hot burn phase, about 500 C and above (comment at `pi:` line 91). Both flags were derived from a regen in one reference log; they have not been validated over many events.
- Battery: `0142` was added on the S3 and is **not yet in the Pi registry**. The 14.0 V running value is from the owner's notes, not from code.
- `dpf_soot_level_g` is the ECU's own estimate; nothing in the logger measures the filter.
- The steering-ECU values are not DPF-relevant; the panel and web tiles do not use them apart from the raw CSV.

## How it was derived

Every decoder was reverse-engineered from a Car Scanner session on this exact car (2026-09-13): Car Scanner's raw ELM327 log (`log.txt`, headers on, echo on) aligned sample for sample with its own decoded CSV export (docstring of `obd/pid_registry.py`, `obd-pi` repo).

1. [`tools/carscanner_parse.py`](../tools/carscanner_parse.py) (this repo) splits the raw log on `>` prompts, matches each echoed request to a name in a hard-coded `REQS` table, and reassembles the ISO-TP payloads (accepting reply ids `7E8` and `7DC`).
2. [`tools/carscanner_solve.py`](../tools/carscanner_solve.py) (this repo) aligns the two series using RPM (a standard formula, u16 x 0.25): it searches the sample offset at which the log-derived RPM and the CSV's `Engine RPM (rpm)` column agree within 1 rpm. It then tries every byte offset and width (u8, i8, u16, i16, u24, u32) of every request, at sample shifts of -2 to +2, keeps candidates with correlation above 0.995, and fits `value = a*x + b`. Columns with fewer than 1000 samples or zero variance are skipped. See [tools/README.md](../tools/README.md).
3. Coefficients were then rounded to sensible values and units checked (metres vs miles).

Validation as recorded in the code docstring: RPM matched 10,447 of 10,447 aligned samples; the other decoders were fitted by linear regression against the CSV columns. I could not re-run this because the Car Scanner export is not in the repository, so those numbers are taken from the docstring.

The two flags are threshold tests on single bytes, not linear fits. Their comments and the project notes say they come from a regen in the reference log; the code does not record how the bytes were chosen.

## What does NOT exist on this ECU

- **Mode 22 (enhanced UDS-style read).** `22280B` returned `7F 22 11` (service not supported) and `22E001` on header `7C6` returned `NO DATA`, tested live 2026-09-19 (comment at `s3:` lines 58-61; the mode-22 result is a code comment, I could not re-test it). A mode-22 PID list supplied by a chatbot was simply wrong for this car. Manufacturer data here is read with **mode 21** instead.
- **Intake manifold pressure (`010B`) and fuel level (`012F`)** are unsupported according to the project notes, so boost cannot be computed on this car (the S3 `turbo_boost` column stays empty). Note that the S3 header comment lists `010B` and `012F` as probes, but the actual table contains different probe rows; the code wins, see below.
- **Probe rows kept in the S3 table** (they back off to one attempt every 30 s after 5 failures): `intake_map_mbar` (`22280B` on 7E0), `baro_kpa` (`01331`), `fuel_level_enh` (`22E001` on 7C6), `fuel_rail_kpa` (`01231`). Whether `0133` and `0123` are answered is not stated anywhere in the code or notes I read.
- Car Scanner's calculated boost, fuel rate/used and engine power are not ECU data and are not logged.

## Known documentation drift

- `s3:` header comment names probes `010B`, `0133`, `012F`, `0123`; the table has `22280B`, `0133`, `22E001`, `0123`. Trust the table.
- `src/config.h` (`obd-esp32` repo) says rows are stamped with milliseconds only and have no wall clock; the code writes both `millis` and `unix_time` (from NTP or the phone). Trust the code.

## Adding a PID

See [contributing.md](contributing.md#adding-a-pid-the-car-scanner-method). The rule is that nothing becomes a logged column until confirmed against a reference capture.
