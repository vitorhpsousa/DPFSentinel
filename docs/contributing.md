# Contributing

This is a hobby project verified on one car. The most valuable contributions are verified PIDs for other engines and fixes to the known issues listed at the end. The guiding rule:

> **Nothing becomes a logged column until it has been confirmed against a real capture from the car.** PID lists from chatbots and forums have been wrong before: a chatbot-supplied mode 22 list was useless because the ix35 has no mode 22.

## Read-only, always

The logger only asks for readings. Do not add requests that clear codes (mode 04), write, change diagnostic session or reprogram anything. Any discovery tooling for another car must likewise be read-only. Anything that could change ECU state does not belong in this repository.

## Dev setup

This repository is PlatformIO/C++ only (root `src/` plus `boards/*/firmware/`).
The one exception is the three "adapt to another car" scripts —
[`tools/discover.py`](../tools/discover.py), [`tools/carscanner_parse.py`](../tools/carscanner_parse.py),
[`tools/carscanner_solve.py`](../tools/carscanner_solve.py) — plain Python
with no dependency on either project's app code; see [tools/README.md](../tools/README.md).
`discover.py` needs only `pyserial`; `carscanner_solve.py` additionally needs
`numpy`:

```bash
python3 -m venv venv
venv/bin/pip install pyserial numpy
```

Firmware (this repo): PlatformIO, `pio run` from the repo root for the original build (see [esp32-s3.md](esp32-s3.md)), or from each board's own directory under `boards/` for the CYD 3.5" and Waveshare targets. Provide your own `src/config.h` values; never commit real credentials.

The separate, still-private `obd-pi` repo has its own Python dev setup (venv,
`requirements.txt`, a Pi logger, dashboard and soot-panel renderer) that does
not apply here; see [cross-project-notes.md](cross-project-notes.md#dev-setup-tests-and-the-astra-discovery-tooling-from-contributingmd)
for what was carried over from it.

## Tests

There are **no automated tests** in this repository yet. The separate, still-private
`obd-pi` repo has its own pytest suite covering its Python equivalents of
`isotpExtract`/`dec_rpm` (`obd.isotp.extract_payload`/`obd.pid_registry.dec_rpm`);
see [cross-project-notes.md](cross-project-notes.md#testing-the-pi-projects-pytest-suite)
for what it covers. It is not a substitute for tests in this repository.

<!-- DECISION NEEDED: this repo's C++ equivalents (`src/obd/isotp.cpp` /
`isotpExtract`, `src/obd/pid_decode.cpp` / `dec_rpm`) have no test suite of
their own. Should this repo add one (needs picking a framework — e.g.
PlatformIO's `pio test` with Unity — since none is set up yet)? That is a
real implementation task, not a docs fix, so it is left open here rather
than invented. -->

Candidates for a future C++ suite in this repo: single/multi-frame reassembly
cases for `src/obd/isotp.cpp`, every decoder in `src/obd/pid_decode.cpp`
against a payload taken from a real `raw_N.log`, and replaying a captured
session through `RegenWatch` (the author says a native replay harness for
`src/report/regen_watch.cpp` exists but it is not in this repo).

## Adding a PID (the Car Scanner method)

1. **Capture a reference.** Run Car Scanner (or another app that can export its raw traffic) with the same adapter on a real drive of 40 minutes or more. Export the raw ELM327 log (`log.txt`, headers on, echo on) and the decoded CSV. They must come from the same session. Do not put these exports, or your own logs, in the repository without checking them (see Privacy).
2. **Teach the parser your requests.** [`tools/carscanner_parse.py`](../tools/carscanner_parse.py) (this repo) has a hard-coded `REQS` table mapping each echoed request *including its frame-count digit* (for example `219480012`, `2103B`) to a name, and it only accepts reply ids `7E8` and `7DC`. Add your new request there. `carscanner_solve.py` also needs the request `010C` (RPM) in the log to align the two files, and a CSV column literally named `Engine RPM (rpm)`.
3. **Solve.** `python3 tools/carscanner_solve.py log.txt export.csv` (this repo; see [tools/README.md](../tools/README.md)). For each CSV column it tries every byte offset and width (u8, i8, u16, i16, u24, u32) of every request at sample shifts of -2 to +2, keeps candidates with correlation above 0.995 and fits `value = a*x + b`. Columns with fewer than 1000 samples or no variance are skipped.
4. **Read the result critically.** Round coefficients to sensible values, check units (the car reports metres, the logger shows miles), and look at the residual error and sample count printed for each candidate. A high correlation with a strange scale is often the wrong byte.
5. **Add the decoder to this repo's registry:**
   - `src/obd/pid_registry.h` and `pid_decode.cpp` (this repo): a decoder and a `PID_TABLE` row, including the poll period.
   - `rxIdFor()` (this repo) if the ECU replies from a new id.
   - Keeping the separate, private `obd-pi` repo's own `obd/pid_registry.py` in sync is a maintainer-only, cross-repo step, not something an outside contributor to this public repo can do — see [cross-project-notes.md](cross-project-notes.md#pid-map-the-pis-pi-citations).
6. **Prove it in your own logger.** Log a drive, compare your CSV against a second Car Scanner run, and check the raw log for the request. Column names are effectively an API (dashboard, panel and Telegram text look them up by name), so rename with care. Files are self-describing because the header row lists every column, but older files will not have your new column.
7. **Document it** in [pid-map.md](pid-map.md): request, header, frame digit, offset, scale, how it was validated and how many samples.

For a different car, start with read-only discovery tooling on that car (rather than pasting a PID list from the internet): run [`tools/discover.py`](../tools/discover.py) (this repo; needs only `pyserial`) against a serial-connected adapter — see [tools/README.md](../tools/README.md) and the project's own notes on adapting to another vehicle in [adapting/other-cars.md](adapting/other-cars.md).

## Privacy

Raw logs and CSVs contain your odometer reading and timestamps. Discovery output (mode 09) contains the **VIN**, and a photo or filename may contain the registration plate. Redact these before sharing a log in an issue. Never commit adapter Bluetooth addresses, Wi-Fi passwords, or Telegram tokens and chat ids.

## Sign off your commits

By contributing you certify the [Developer Certificate of Origin](../DCO) —
in short, that you wrote the contribution yourself or otherwise have the
right to submit it under this project's licence. Add your certification to
every commit with `git commit -s`, which appends a line like:

```
Signed-off-by: Random J Developer <random@developer.example.org>
```

There's no automated check for this yet (contributions are rare enough that
it's reviewed by hand for now), but sign off anyway — it's what the record
of who-wrote-what actually relies on.

## Known issues you could fix

<!-- Several issues that used to be listed here (webui/server.py's build_schema(), soot_panel.py's missing
     requirements.txt deps, OBD_BT_MAC in config.py, control_module_v missing from the Pi registry,
     the hard-coded systemd user/path, and tools/log_decoder.py) are about `obd-pi` code that does not
     exist in this repository, so they were removed from this list as not actionable here — see obd-pi's
     own issue tracker for those. -->
- Idle-blockage flag ignores engine temperature, so it is not really a "hot idle" test.
- `colOf()` in `src/main.cpp` can return -1 and is then used as an index.
- The timezone is hard-coded for the UK in the S3 firmware.
