# Adapting-to-another-car tools

None of this is needed for the main easy-setup path on the verified car
(2014 Hyundai ix35 1.7 CRDi). These three scripts exist only for someone
adapting this project to a **different** car, where the PID map in
[../docs/pid-map.md](../docs/pid-map.md) does not apply and the request
bytes, offsets and scaling have to be found again from scratch. See
[../docs/adapting/other-cars.md](../docs/adapting/other-cars.md) for the
full write-up; this file is just what to run and in what order.

All three are read-only: they only ever ask for data (OBD services 01, 03,
07, 09, 0A, 22 and AT/ST adapter commands). Nothing here writes to an ECU,
clears codes, or changes a diagnostic session.

## 1. `discover.py` — find out what your car supports

Run first, against a serial-connected ELM327/STN-type adapter (ignition on,
engine off to start). It identifies the protocol, lists every supported
standard PID, reads VIN/calibration id, checks which ECUs answer, reads
fault codes, samples the standard PIDs while you rev the engine, then
probes a list of community-sourced manufacturer-mode (mode 22) candidates
that sometimes carry DPF-related data on other vehicles. Everything sent
and received is written to a timestamped log file.

```bash
pip install pyserial
python3 discover.py --device /dev/rfcomm0 --sample-seconds 60
```

`--device` is any serial port: a Bluetooth RFCOMM bind (`/dev/rfcomm0` on
Linux), a USB-serial adapter (`/dev/ttyACM0`, `/dev/ttyUSB0`), or a Windows
COM port (`COM3`). `--unattended` keeps retrying until the adapter and car
answer, then waits for the engine to start before sampling — useful if you
cannot watch a screen the whole time. The only dependency beyond the Python
standard library is `pyserial`.

The mode 22 probe list is **UNVERIFIED community data** carried over from
forum threads on two other diesel models — treat every hit as something to
confirm with a real capture (next step), never as a fact for your car.

## 2. Log a session with the Car Scanner app

Once you know roughly what to look for, connect the [Car Scanner](https://play.google.com/store/apps/details?id=com.cardiagnostics.carscanner)
phone app to the car with the same adapter, enable "save raw log" in its
settings, add the parameters you care about to a screen, and drive for 40
minutes or more (a DPF regeneration event, if you can catch one, is worth
a lot). Export two files from the same session:

- the raw ELM327 log (`log.txt`, headers on, echo on)
- the decoded CSV export

Keep both offline — they contain your odometer and timestamps.

## 3. `carscanner_parse.py` + `carscanner_solve.py` — derive exact byte offsets and scaling

These two turn the raw log and the CSV from step 2 into a specific byte
offset, width and linear scale for each value, by cross-referencing the two
files against each other.

`carscanner_parse.py` splits the raw log into per-request reassembled
ISO-TP payloads. It needs a hard-coded `REQS` table (in the script) mapping
each echoed request — including its ELM327 frame-count digit — to a name;
add your own car's requests there before running `carscanner_solve.py`.
It can also be run directly for a quick look:

```bash
python3 carscanner_parse.py log.txt
```

`carscanner_solve.py` (needs `numpy`: `pip install numpy`) aligns the parsed
log against the CSV using engine RPM (a standard formula), then tries every
byte offset and width (u8, i8, u16, i16, u24, u32) of every request at a few
sample-alignment shifts, keeping only candidates with correlation above
0.995, and fits `value = a*x + b`:

```bash
python3 carscanner_solve.py log.txt export.csv
```

Read the output critically — a strong correlation with an odd-looking scale
is often the wrong byte. Round coefficients to sensible values and check
units (this project's reference car reports metres but shows miles).

## After this

Once you have a confirmed request, header, offset and scale for a value,
add it to the firmware's PID table (`src/obd/pid_registry.h` +
`pid_decode.cpp`, and the matching files under each `boards/*/firmware/src/obd/`
if it should exist on those targets too) and document it in
[../docs/pid-map.md](../docs/pid-map.md) alongside how it was verified. See
[../docs/contributing.md](../docs/contributing.md) ("Adding a PID") for the
full checklist.
