# Contributing

This is a hobby project verified on one car. The most valuable contributions are verified PIDs for other engines and fixes to the known issues listed at the end. The guiding rule:

> **Nothing becomes a logged column until it has been confirmed against a real capture from the car.** PID lists from chatbots and forums have been wrong before: a chatbot-supplied mode 22 list was useless because the ix35 has no mode 22.

## Read-only, always

The logger only asks for readings. Do not add requests that clear codes (mode 04), write, change diagnostic session or reprogram anything. The discovery tooling in `astra/` is likewise read-only. Anything that could change ECU state does not belong in this repository.

## Dev setup

Python parts (Pi logger, dashboard, tools):

```bash
python3 -m venv venv
venv/bin/pip install -r pi/requirements.txt        # pyserial
venv/bin/pip install numpy                          # only for tools/carscanner_solve.py
venv/bin/pip install requests pillow                # only for pi/screen/soot_panel.py
```

Run from `pi/` (modules import `config` and `obd.*` relative to it): `python3 main.py` needs an adapter; `python3 webui/server.py` runs against any directory of `session_N.csv` files (set `LOG_DIR` in `pi/config.py`). The soot panel looks for DejaVu fonts in `/usr/share/fonts/truetype/dejavu`; without them it falls back to a tiny bitmap font that looks broken. On a desktop, import `render()` from `screen/soot_panel.py` and save the returned image instead of writing `/dev/fb0`.

Firmware: PlatformIO, `cd esp32-s3-ble && pio run` (see [esp32-s3.md](esp32-s3.md)). Provide your own `src/config.h` values; never commit real credentials.

## Tests

There are **no automated tests** in the repository yet. Good first additions, using values checked while writing these docs:

```python
# run from pi/
from obd.isotp import extract_payload
from obd.pid_registry import dec_rpm

def test_single_frame_rpm():
    payload = extract_payload("7E804410C1AF8", "7E8")   # 7E8 04 41 0C 1A F8
    assert payload.hex() == "410c1af8"
    assert dec_rpm(payload) == 1726.0

def test_wrong_id_is_ignored():
    assert extract_payload("7E804410C1AF8", "7DC") is None
```

Other candidates: a multi-frame reassembly case, every decoder against a payload taken from a real `raw_N.log`, and replaying a captured session through `RegenWatch` (the author says a native replay harness for `esp32-s3-ble/src/report/regen_watch.cpp` exists but it is not in the repo).

## Adding a PID (the Car Scanner method)

1. **Capture a reference.** Run Car Scanner (or another app that can export its raw traffic) with the same adapter on a real drive of 40 minutes or more. Export the raw ELM327 log (`log.txt`, headers on, echo on) and the decoded CSV. They must come from the same session. Do not put these exports, or your own logs, in the repository without checking them (see Privacy).
2. **Teach the parser your requests.** `tools/carscanner_parse.py` has a hard-coded `REQS` table mapping each echoed request *including its frame-count digit* (for example `219480012`, `2103B`) to a name, and it only accepts reply ids `7E8` and `7DC`. Add your new request there. `carscanner_solve.py` also needs the request `010C` (RPM) in the log to align the two files, and a CSV column literally named `Engine RPM (rpm)`.
3. **Solve.** `python3 tools/carscanner_solve.py log.txt export.csv`. For each CSV column it tries every byte offset and width (u8, i8, u16, i16, u24, u32) of every request at sample shifts of -2 to +2, keeps candidates with correlation above 0.995 and fits `value = a*x + b`. Columns with fewer than 1000 samples or no variance are skipped.
4. **Read the result critically.** Round coefficients to sensible values, check units (the car reports metres, the logger shows miles), and look at the residual error and sample count printed for each candidate. A high correlation with a strange scale is often the wrong byte.
5. **Add the decoder to both registries**, keeping them in sync by hand:
   - `pi/obd/pid_registry.py`: a `dec_*` function and a `PidDef(name, unit, header, request_hex, frames, prefix, decode)` row. Offsets are counted from the service byte (`[0]` = `0x41`/`0x61`).
   - `esp32-s3-ble/src/obd/pid_registry.h` and `pid_decode.cpp`: a decoder and a `PID_TABLE` row, including the poll period.
   - `RX_ID` (Pi) / `rxIdFor()` (S3) if the ECU replies from a new id.
6. **Prove it in your own logger.** Log a drive, compare your CSV against a second Car Scanner run, and check the raw log for the request. Column names are effectively an API (dashboard, panel and Telegram text look them up by name), so rename with care. Files are self-describing because the header row lists every column, but older files will not have your new column.
7. **Document it** in [pid-map.md](pid-map.md): request, header, frame digit, offset, scale, how it was validated and how many samples.

For a different car, start with `astra/discovery_pi/discover.py` (read-only) to find protocol, supported PIDs and modules rather than pasting a PID list from the internet. `astra/design/reuse_plan.md` sketches a profile-driven refactor; none of it is implemented and nothing about the second car is verified.

## Privacy

Raw logs and CSVs contain your odometer reading and timestamps. Discovery output (mode 09) contains the **VIN**, and a photo or filename may contain the registration plate. Redact these before sharing a log in an issue. Never commit adapter Bluetooth addresses, Wi-Fi passwords, or Telegram tokens and chat ids.

## Known issues you could fix

- `pi/webui/server.py` `build_schema()` reads `p.confirmed`, which `PidDef` no longer has, so `/api/schema` raises; `app.js` uses it for an "unconfirmed" hint.
- `soot_panel.py` needs `requests` and `pillow` that are missing from `pi/requirements.txt`.
- `OBD_BT_MAC` in `pi/config.py` is unused; the bind script keeps its own `MAC`. Read it from one place.
- `control_module_v` (PID 0142) is not in the Pi registry.
- Idle-blockage flag ignores engine temperature, so it is not really a "hot idle" test.
- `colOf()` in `esp32-s3-ble/src/main.cpp` can return -1 and is then used as an index.
- systemd units and defaults hard-code user `ix35` and `/home/ix35/...`.
- The timezone is hard-coded for the UK in the S3 firmware.
- `tools/log_decoder.py` is an older, minimal helper (its docstring is written around mode 22 and a `request -> response` sniff format); it does not decode anything automatically and may be worth removing.
