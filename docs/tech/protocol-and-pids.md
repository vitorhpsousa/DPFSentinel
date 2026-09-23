# Protocol and PIDs (technical reference)

Scope: the Pi logger (`pi/`). Paths are relative to the repository root. Labels used below: **[code]** read directly from the source, **[measured]** stated by a code comment or project note as observed on the car (not re-testable from this repo), **[inferred]** fitted or guessed, **[unverified]** not checked. The user-level table of columns is in [../pid-map.md](../pid-map.md); this page explains the machinery behind it.

## Layers

```
PID_TABLE row (pi/obd/pid_registry.py)
  -> ElmClient.set_header()  ATSH<hdr>          (pi/obd/elm_client.py)
  -> ElmClient.query_raw()   "<request><digit>\r"
  -> extract_payload()       ISO-TP reassembly   (pi/obd/isotp.py)
  -> prefix check, decode()  fixed offsets + scale
```

## ELM327 conversation

[code] `ElmClient.send_command` (`pi/obd/elm_client.py`): clears the input buffer, writes `cmd + "\r"` (ASCII), then polls `read(64)` (non-blocking, `timeout=0`; sleeps 10 ms when empty) until a chunk contains `>` or the deadline passes (default 2.0 s; `query_raw` uses 3.0 s). It strips `>` and whitespace and returns the text. It never raises on timeout; a timeout yields an empty or partial string.

Init sequence, `init_adapter()`: `ATZ` (3 s allowance), then `ATE0` (echo off), `ATL0` (linefeeds off), `ATH1` (headers on), `ATS0` (spaces off), `ATSP6` (ISO 15765-4, CAN 11-bit ID, 500 kbit/s), `ATST96` (reply timeout 0x96 x 4 ms = 600 ms). An empty response to any of them makes `init_adapter()` return `False`; `main.connect_adapter` then only prints a warning and carries on.

- `ATSP6` instead of `ATSP0`: [measured, per code comment] `ATDPN` on this car returned protocol 6, and auto-search took 8-10 s on the first query, longer than the logger's own timeout, so every request silently failed.
- The S3 firmware uses `ATST32` (200 ms) instead of `ATST96` [code: `esp32-s3-ble/src/obd/elm_client.cpp` line 247].

Header switching: `set_header(h)` sends `ATSH<h>` and expects `OK` in the reply; it caches the current header and skips the command if unchanged. Note the cache is not cleared by `init_adapter()`; this only matters if init is re-run on a live `ElmClient`, which `main.py` never does (it builds a new client on reconnect) [code].

## Request format and the frame-count digit

[code] `query_raw(request_hex, frames)` sends `request_hex + frames`. The extra hex digit is the ELM327 "expected number of response lines" feature: the adapter stops waiting as soon as that many frames arrive. `B` = 11. [measured, per docstrings] Car Scanner sends the same digits on this car and the logger copies it. A clone adapter that does not understand the digit is [unverified] on the Pi; the S3 firmware retries without it (per `../pid-map.md`, not re-read here).

## Headers and reply IDs

`RX_ID` in `pi/obd/pid_registry.py` [code]:

| Transmit (ATSH) | Meaning | Reply CAN id filtered on |
|---|---|---|
| `7DF` | functional (broadcast) request, mode 01 | `7E8` |
| `7E0` | ECM physical address, mode 21 | `7E8` |
| `7D4` | steering (EPS) ECU | `7DC` |

A header not in `RX_ID` raises `KeyError` from `PidDef.rx_id`. Reply ids for `7D4 -> 7DC` follow the usual "+8" convention; that the ix35's EPS really is addressed at `7D4` is [measured] only via Car Scanner's log (docstring).

## ISO-TP reassembly (`pi/obd/isotp.py`)

Input is text with headers on, spaces off, e.g. `7E8034104A2` (single frame) or `7E8104B6103FFFFFFFF\r7E821...` (first + consecutive frames).

[code] Behaviour of `extract_payload(text, rx_id)`:

1. Split on `\r`/`\n`. A line is kept only if it is >= 5 characters, has an even number of characters after the 3-character id, is pure hex, and its first three characters equal `rx_id`. This drops echoes (`2103B` is 5 chars but its "id" is `210`), `SEARCHING...`, `NO DATA`, and frames from other ECUs.
2. The **first kept frame** decides the type by its PCI nibble:
   - `0` single frame: length = low nibble; returns those bytes, `None` if short or length 0.
   - `1` first frame: total length = 12-bit value in bytes 0-1; data starts with the remaining 6 bytes; every later frame whose PCI nibble is `2` is appended; returns `total` bytes, or `None` if fewer arrived.
   - anything else (flow control, etc.): `None`.
3. Consecutive-frame **sequence numbers are not checked** and duplicates/out-of-order frames are not detected [code]. Trailing pad bytes are cut off by `total`.
4. The logger never sends an ISO-TP flow-control frame; it relies on the adapter doing that (standard ELM327 behaviour, [unverified] for each clone).

`poll_cycle` (`pi/main.py`) then requires `payload.startswith(pid.prefix)`; otherwise the value is `None` (blank CSV cell).

Worked example [code, exercised by tests]: `7E804410C1AF8` -> payload `41 0C 1A F8` -> `dec_rpm` reads u16 at offset 2 = `0x1AF8` = 6904, x 0.25 = 1726 rpm.

## Decoder conventions

Offsets are from the start of the payload, i.e. byte 0 is the service echo (`0x41` for mode 01, `0x61` for mode 21) and byte 1 the PID/local id. Helpers `_u8/_u16/_i16/_u24/_u32` return `None` if the payload is too short, so short replies give blanks rather than exceptions [code].

## PID table

One row per column in `PID_TABLE`; requests shared by several rows are sent once per cycle, keyed by `(header, request_hex)` [code: `poll_cycle`]. Status column: **M** = value formula matched a Car Scanner export by regression per the `pid_registry.py` docstring [measured; export not in repo, so not re-runnable]; **S** = standard SAE J1979 formula [code: matches the standard definitions]; **F** = flag from a single-byte threshold [inferred].

| Column | Hdr | Request(+digit) | Prefix | Decode | Status |
|---|---|---|---|---|---|
| engine_rpm | 7DF | `010C`+1 | 41 0C | u16@2 x 0.25 | S, M |
| coolant_temp | 7DF | `0105`+1 | 41 05 | u8@2 - 40 | S |
| vehicle_speed | 7DF | `010D`+1 | 41 0D | u8@2 | S |
| maf_flow | 7DF | `0110`+1 | 41 10 | u16@2 x 0.01 | S |
| engine_load_pct | 7DF | `0104`+1 | 41 04 | u8@2 x 100/255 | S |
| egr_duty_pct | 7DF | `012C`+1 | 41 2C | u8@2 x 100/255 | S |
| intake_air_temp_c | 7DF | `010F`+1 | 41 0F | u8@2 - 40 | S |
| dpf_zone_temp_c | 7DF | `013E`+1 | 41 3E | u16@2 x 0.1 - 40 | S (catalyst temp B1S2, not a DPF sensor; name kept for continuity) |
| dpf_diff_pressure_hpa | 7E0 | `211B`+1 | 61 1B | u16@2 x 0.5 | M |
| dpf_soot_level_g | 7E0 | `21948001`+2 | 61 | u8@10 x 100/255 | M |
| intercooler_temp_c | 7E0 | `21948001`+2 | 61 | u8@6 - 50 | M |
| egt_before_dpf_c | 7E0 | `2103`+B | 61 03 | u16@48 x 0.0183966 + 99.013 | M |
| dpf_dist_since_regen_mi | 7E0 | `2103`+B | 61 03 | u24@56 x 0.000621371 | M |
| odometer_mi | 7E0 | `2103`+B | 61 03 | u32@59 x 0.000621371 | M |
| dpf_regen_active | 7E0 | `2103`+B | 61 03 | byte@71 == 4 | F |
| dpf_regen_burning | 7E0 | `2103`+B | 61 03 | byte@19 == 21 | F |
| eps_speed_kmh | 7D4 | `2101`+3 | 61 01 | u8@5 | M |
| eps_steering_deg | 7D4 | `2101`+3 | 61 01 | i16@11 x 0.1 | M |
| eps_voltage_v | 7D4 | `2101`+3 | 61 01 | u8@3 x 0.1 | M |
| dpf_odo_at_last_regen_mi | derived | none | | `odometer_mi - dpf_dist_since_regen_mi` | code (`main.py` `poll_cycle`) |

Notes:

- Distances are converted from metres to miles (`0.000621371`): the source car is UK-market with a mile dashboard [inferred from units in the reference CSV; the code only stores the factor].
- The flags have no derivation recorded in code. The comments say `regen_active` stays 1 from warm-up until the distance counter resets and `regen_burning` is the hot phase (EGT roughly above 500 C). [inferred from one reference regen; **not validated over multiple events**.]
- Soot resolution is one byte: 100/255 = 0.392 g per step [code].
- Whether "soot in grams" is physically accurate is unknown; it is the ECU's own model value.
- Mode 22 does not exist on this ECU (`22280B` -> `7F 22 11`; `22E001` on 7C6 -> no data) [measured 2026-09-19, source is a comment in `esp32-s3-ble/src/obd/pid_registry.h`; not re-tested here]. `010B` and `012F` reported unsupported [measured per project notes, not in code].
- The Pi table has no `0142` battery voltage; the S3 does.

## Measured vs inferred summary

| Claim | Basis |
|---|---|
| Protocol is ISO 15765-4 CAN 11/500 | measured via `ATDPN` (comment) |
| Formulas for M rows | regression against Car Scanner export, single session 2026-09-13; RPM matched 10,447 of 10,447 samples (docstring figure) |
| Regen flag bytes | inferred from one regen |
| Idle-blockage thresholds (20 hPa, 1100 rpm) | unvalidated defaults in `pi/config.py` |
| Mode 21 blocks other than `2103`, `21948001`, `211B`, `2101` | not explored in this repo |

## Reproducing the derivation

`tools/carscanner_parse.py` (split the raw log on `>`, match echoed requests to a hard-coded `REQS` table, reassemble payloads) and `tools/carscanner_solve.py` (align RPM, then fit every byte offset/width at shifts -2..+2, keep correlation > 0.995, fit `a*x+b`). See [../contributing.md](../contributing.md) for the workflow. [unverified] here: I did not run these tools.
