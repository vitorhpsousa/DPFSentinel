# OBD DPF Logger

A small, read-only data logger that watches the **diesel particulate filter (DPF)** of a 2014 Hyundai ix35 1.7 CRDi (engine D4FD) through an OBD-II adapter. It records the engine computer's own DPF figures once a second, shows them live, and (on the ESP32 build) sends a message when a regeneration starts, finishes or is interrupted.

> **Not a diagnostic tool.** It reads numbers the car already reports and cannot tell you the filter is blocked. Do not rely on it for safety or repair decisions. See [docs/disclaimer.md](docs/disclaimer.md).

<!-- screenshot: web dashboard, live tab -->
<!-- screenshot: 3.5" soot panel (green / amber / dark red / regen banner) -->
<!-- photo: hardware in the car -->

## Why

A DPF traps soot and cleans itself by burning it off (a regeneration, "regen") when the exhaust gets hot enough for long enough. Short trips never get there, so soot keeps climbing. Two signals matter:

- **The ECU's soot estimate** (grams). On the author's ix35 it rose from 12.2 g to 17.6 g over two short rides, the top of the range seen in the Car Scanner app.
- **Differential pressure across the filter, at hot idle.** Pressure at motorway speed is high for normal reasons; a filter that is filling shows a higher pressure than a clean one at the *same* warm idle. The logger records the pressure so a personal baseline can be built. The bundled 20 hPa idle flag is an unverified placeholder, not a calibrated limit (see [What works](#what-works-and-what-is-unverified)).

Existing phone apps show this live but do not keep a history you control. This project keeps every drive as a CSV plus the raw adapter replies, so trends over days and weeks can be inspected.

## Supported targets

New to this repo? Start with [docs/getting-started.md](docs/getting-started.md) for a software checklist per target.

| Target | State | Docs |
|---|---|---|
| Raspberry Pi 4 + OBDLink MX+ (Bluetooth Classic) | Primary path, used on a real car | [docs/pi-setup.md](docs/pi-setup.md) |
| Raspberry Pi 3 | See notes | [docs/pi3-notes.md](docs/pi3-notes.md) |
| ESP32-S3 + BLE ELM327 adapter | Working on the author's car | [docs/esp32-s3.md](docs/esp32-s3.md), [docs/telegram-alerts.md](docs/telegram-alerts.md) |
| ESP32-C3 / C6 | Planned, not built | [docs/esp32-c3-c6.md](docs/esp32-c3-c6.md) |

Only **one car** (the ix35) has ever been verified. The PID map is specific to its engine computer; another car needs its own discovery and verification work ([docs/contributing.md](docs/contributing.md)). An AI-assisted setup walkthrough is in [docs/ai-setup-prompt.md](docs/ai-setup-prompt.md); to host the repository yourself see [docs/self-hosting-gitea.md](docs/self-hosting-gitea.md).

## What works and what is unverified

Verified on the ix35 (decoders fitted against Car Scanner's raw ELM327 log and its decoded CSV; see [docs/pid-map.md](docs/pid-map.md)):

- Standard mode 01: RPM, coolant, speed, MAF, load, EGR duty, intake air temperature, catalyst temperature.
- Manufacturer mode 21 on the engine ECU: DPF differential pressure, soot level, intercooler temperature, exhaust temperature before the DPF, distance since last regen, odometer, regen flags.
- Steering ECU (7D4) values: speed, steering angle, voltage.
- Battery/module voltage (PID 0142), S3 build only.
- Pi logger: logging, in-process reconnect, CSV plus raw log, web dashboard, 3.5" soot panel.
- ESP32-S3: logging to SD, web UI, NTP or phone clock, Telegram report and regen alerts.

Not verified or not available:

- The **hot-idle pressure baseline** and the 20 hPa / 1100 rpm idle flag: thresholds are guesses; the code does not even check that the engine is warm.
- The **regen flags** were derived from a regen in one reference log, not a long history of events. Live regen capture with the new logger is still outstanding.
- The **soot colour thresholds** on the 3.5" panel (14 g / 17 g / 28 g) are the owner's choices, not a specification. Soot is the ECU's estimate, not a measurement of the filter.
- **Mode 22 does not exist on this ECU**, and intake MAP (010B) and fuel level (012F) are unsupported, so boost cannot be computed.
- `dpf_zone_temp_c` is really the catalyst temperature (PID 013E), not the DPF temperature. See [docs/pid-map.md](docs/pid-map.md).
- Nothing about any other car (a 2014 Vauxhall Astra is being researched) has been tested.
- There are no automated tests in the repository.

## Hardware

Pi path:

- Raspberry Pi 4 with Raspberry Pi OS (BlueZ), a way to power it in the car (not covered here)
- OBDLink MX+ (Bluetooth Classic SPP)
- Optional: 3.5" MHS3528 SPI touchscreen (ILI9486 display, XPT2046 touch); the panel app only draws, it ignores touch

ESP32-S3 path:

- ESP32-S3 board with a microSD slot (developed on a Freenove ESP32-S3-WROOM, PlatformIO board `esp32-s3-devkitc-1`)
- A BLE ELM327 adapter (developed on a Konnwei)
- microSD card (8 GB FAT32 was used)

The S3 has no Bluetooth Classic radio, so it cannot use the MX+ the way the Pi does (Classic SPP). Whether an MX+'s BLE mode works with the S3 firmware has not been tried.

## Quick start (Pi)

Full detail in [docs/pi-setup.md](docs/pi-setup.md). In short:

```bash
git clone <repo-url> obd-logger && cd obd-logger
sudo apt install bluez python3-venv
python3 -m venv venv
venv/bin/pip install -r pi/requirements.txt      # pyserial

# 1. Pair and trust the adapter once with bluetoothctl (see pi-setup.md)
# 2. Put your adapter's address into pi/scripts/bind_rfcomm.sh (variable MAC)
# 3. Adjust LOG_DIR in pi/config.py
sudo pi/scripts/bind_rfcomm.sh                    # creates /dev/rfcomm0
cd pi && ../venv/bin/python3 main.py              # ignition on
# second terminal:
../venv/bin/python3 webui/server.py               # http://<pi-ip>:8080/
```

A new `session_N.csv` and `raw_N.log` appear in `LOG_DIR`. With the engine off some requests will legitimately return no data and cells stay empty.

## Repository layout

| Path | Contents |
|---|---|
| `pi/` | Raspberry Pi logger, dashboard, screen panel, systemd units |
| `esp32-s3-ble/` | ESP32-S3 BLE firmware (PlatformIO) |
| `esp32/` | Earlier plain-ESP32 firmware, state unknown, undocumented |
| `tools/` | Car Scanner log parser/solver, clock backfill, log helper |
| `astra/` | Research and untested discovery tooling for a second car |
| `docs/` | Documentation |

## Documentation

- [Architecture](docs/architecture.md): data flow, log formats, columns, services
- [PID map](docs/pid-map.md): every request, offset and scale, and how each was derived
- [Pi setup](docs/pi-setup.md) and [Pi 3 notes](docs/pi3-notes.md)
- [ESP32-S3](docs/esp32-s3.md), [Telegram alerts](docs/telegram-alerts.md), [ESP32-C3/C6](docs/esp32-c3-c6.md)
- [Contributing](docs/contributing.md)
- [Disclaimer](docs/disclaimer.md)

## Secrets

The firmware config (`esp32-s3-ble/src/config.h`) holds Wi-Fi passwords and a Telegram token, and the adapter's Bluetooth address appears in `pi/config.py` and `pi/scripts/bind_rfcomm.sh`. Use your own values and never commit them.

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE).
