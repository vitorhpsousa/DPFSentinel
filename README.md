# DPF Sentinel — ESP32 OBD DPF Logger

A small, read-only data logger that watches the **diesel particulate filter (DPF)** of a 2014 Hyundai ix35 1.7 CRDi (engine D4FD) through an OBD-II adapter, on ESP32-S3 hardware. It records the engine computer's own DPF figures once a second, shows them live on a web dashboard and (on the touchscreen boards) a colour-coded panel, and sends a Telegram message when a regeneration starts, finishes or is interrupted.

> **Not a diagnostic tool.** It reads numbers the car already reports and cannot tell you the filter is blocked. Do not rely on it for safety or repair decisions. See [docs/disclaimer.md](docs/disclaimer.md).

This repository holds the **ESP32-S3 firmware only** (three targets, below). A Raspberry Pi 4 version of the same logger exists as a separate project and is not yet public.

<!-- screenshot: web dashboard, live tab -->
<!-- screenshot: 3.5" soot panel (green / amber / dark red / regen banner) -->
<!-- photo: hardware in the car -->

## Why

A DPF traps soot and cleans itself by burning it off (a regeneration, "regen") when the exhaust gets hot enough for long enough. Short trips never get there, so soot keeps climbing. Two signals matter:

- **The ECU's soot estimate** (grams). On the author's ix35 it rose from 12.2 g to 17.6 g over two short rides, the top of the range seen in the Car Scanner app.
- **Differential pressure across the filter, at hot idle.** Pressure at motorway speed is high for normal reasons; a filter that is filling shows a higher pressure than a clean one at the *same* warm idle. The logger records the pressure so a personal baseline can be built. The bundled 20 hPa idle flag is an unverified placeholder, not a calibrated limit (see [What works](#what-works-and-what-is-unverified)).

Existing phone apps show this live but do not keep a history you control. This project keeps every drive as a CSV plus the raw adapter replies, so trends over days and weeks can be inspected.

## Supported targets

**New to all this and not a programmer?** [docs/easy-setup/](docs/easy-setup/README.md) is a point-and-click guide to building the main in-car board (CYD 3.5") with no command-line or developer experience assumed, with or without an AI assistant walking you through it — there's also a [one-file PDF version](docs/easy-setup/pdf/DPF_Sentinel_Easy_Setup_Guide.pdf) to share.

Already comfortable with a terminal? Start with [docs/getting-started.md](docs/getting-started.md) for a software checklist across all three targets below.

| Target | Location | State | Docs |
|---|---|---|---|
| ESP32-S3 + BLE ELM327 adapter (original build) | repo root | Working on the author's car | [docs/esp32-s3.md](docs/esp32-s3.md), [docs/telegram-alerts.md](docs/telegram-alerts.md) |
| CYD 3.5" touchscreen board | [boards/cyd-s3-3p5/](boards/cyd-s3-3p5/) | Main in-car board, working on the author's car | [docs/easy-setup/](docs/easy-setup/README.md), [board notes](boards/cyd-s3-3p5/board_notes.md) |
| Waveshare ESP32-S3-Touch-LCD-2 (2") | [boards/waveshare-s3-touch-lcd-2/](boards/waveshare-s3-touch-lcd-2/) | Earlier bring-up, working; boards this size commonly use a small onboard antenna with no external option, which struggled for BLE range near the OBD adapter in testing — the CYD board is the one actually used day to day in the car | [board notes](boards/waveshare-s3-touch-lcd-2/board_notes.md) |

Only **one car** (the ix35) has ever been verified. The PID map is specific to its engine computer; another car needs its own discovery and verification work ([docs/contributing.md](docs/contributing.md)). An AI-assisted setup walkthrough is in [docs/ai-setup-prompt.md](docs/ai-setup-prompt.md); to host the repository yourself see [docs/self-hosting-gitea.md](docs/self-hosting-gitea.md).

## What works and what is unverified

Verified on the ix35 (decoders fitted against Car Scanner's raw ELM327 log and its decoded CSV; see [docs/pid-map.md](docs/pid-map.md)):

- Standard mode 01: RPM, coolant, speed, MAF, load, EGR duty, intake air temperature, catalyst temperature.
- Manufacturer mode 21 on the engine ECU: DPF differential pressure, soot level, intercooler temperature, exhaust temperature before the DPF, distance since last regen, odometer, regen flags.
- Steering ECU (7D4) values: speed, steering angle, voltage.
- Battery/module voltage (PID 0142).
- All three targets: logging to SD or internal flash, web dashboard, NTP or phone clock, Telegram report and regen alerts.

Not verified or not available:

- The **hot-idle pressure baseline** and the 20 hPa / 1100 rpm idle flag: thresholds are guesses; the code does not even check that the engine is warm.
- The **regen flags** were derived from a regen in one reference log, not a long history of events.
- The **soot colour thresholds** on the touchscreen panels (14 g / 17 g / 28 g) are the owner's choices, not a specification. Soot is the ECU's estimate, not a measurement of the filter.
- **Mode 22 does not exist on this ECU**, and intake MAP (010B) and fuel level (012F) are unsupported, so boost cannot be computed.
- `dpf_zone_temp_c` is really the catalyst temperature (PID 013E), not the DPF temperature. See [docs/pid-map.md](docs/pid-map.md).
- Nothing about any other car (a 2014 Vauxhall Astra is being researched) has been tested.
- There are no automated tests in the repository.

## Hardware

- An ESP32-S3 board. The original build targets a bare Freenove ESP32-S3-WROOM (PlatformIO board `esp32-s3-devkitc-1`); the two touchscreen boards are specific off-the-shelf products — see their own board notes for exact listings.
- A **BLE** OBD-II ELM327 adapter (developed on a Konnwei; Veepeak and similar BLE adapters also match the firmware's auto-detect list). The ESP32-S3 has no Bluetooth Classic radio, so Classic-only adapters (like an OBDLink MX+ in its default mode) will not work here.
- A microSD card, FAT32, 32GB or smaller (the touchscreen boards; the root build can also log to internal flash).

## Quick start (original ESP32-S3 build, repo root)

Full detail in [docs/esp32-s3.md](docs/esp32-s3.md) and [docs/getting-started.md](docs/getting-started.md). In short, with [PlatformIO](https://platformio.org/) installed ([per-OS setup](docs/workstation-setup.md)):

```bash
git clone https://github.com/vitorhpsousa/DPFSentinel.git && cd DPFSentinel
# edit src/config.h with your own WiFi/Telegram values first — see docs/esp32-s3.md
pio run -t upload
pio device monitor
```

For the CYD 3.5" or Waveshare 2" boards instead, see their rows in [Supported targets](#supported-targets) above — each is its own PlatformIO project under `boards/`, not built from the repo root.

A new `session_N.csv` and `raw_N.log` appear each session. With the engine off some requests will legitimately return no data and cells stay empty.

## Repository layout

| Path | Contents |
|---|---|
| `platformio.ini`, `src/` (repo root) | Original ESP32-S3 BLE firmware |
| `boards/cyd-s3-3p5/` | CYD 3.5" touchscreen board: firmware, board notes, bring-up tools |
| `boards/waveshare-s3-touch-lcd-2/` | Waveshare 2" touchscreen board: firmware, board notes, bring-up tools |
| `docs/` | Documentation (see below) |

## Documentation

**Getting started**
- [Easy setup](docs/easy-setup/README.md): no-code, point-and-click guide to the CYD 3.5" board, with an AI-assisted option — also available as a [PDF](docs/easy-setup/pdf/DPF_Sentinel_Easy_Setup_Guide.pdf)
- [Getting started](docs/getting-started.md): software checklist across all three targets, for anyone already comfortable with PlatformIO
- [Workstation setup](docs/workstation-setup.md): installing PlatformIO, serial drivers/permissions per OS
- [Manual setup](docs/manual-setup.md): the whole setup yourself, without an AI agent touching your hardware
- [AI setup prompt](docs/ai-setup-prompt.md): a ready-made prompt for an AI assistant to walk you through any target

**Reference**
- [Architecture](docs/architecture.md): data flow, log formats, columns, services
- [PID map](docs/pid-map.md): every request, offset and scale, and how each was derived
- [ESP32-S3 (original build)](docs/esp32-s3.md), [ESP32-C3/C6](docs/esp32-c3-c6.md) (planned, not built)
- [Telegram alerts](docs/telegram-alerts.md)
- `boards/cyd-s3-3p5/board_notes.md` and `boards/waveshare-s3-touch-lcd-2/board_notes.md`: pin maps and per-board verification notes

**Using it day to day**
- [What it does](docs/user-guide/01-what-it-does.md), [Daily use](docs/user-guide/02-daily-use.md), [Telegram alerts for users](docs/user-guide/03-telegram-alerts-for-users.md), [Troubleshooting](docs/user-guide/04-troubleshooting.md), [FAQ](docs/user-guide/05-faq.md)

**Adapting it to another car or app**
- [Other cars](docs/adapting/other-cars.md), [Other apps](docs/adapting/other-apps.md), [PID profile format](docs/adapting/pid-profile-format.md)

**For garages**
- [What a garage gets](docs/garage/01-what-a-garage-gets.md), [Install in 10 minutes](docs/garage/02-install-in-10-minutes.md), [Reading the data](docs/garage/03-reading-the-data.md), [Customer handout](docs/garage/04-customer-handout.md), [Legal and liability](docs/garage/05-legal-and-liability.md), [Business models](docs/garage/06-business-models.md)

**Project**
- [Contributing](docs/contributing.md)
- [Disclaimer](docs/disclaimer.md)
- [Licensing](docs/licensing/options.md): the reasoning behind the chosen licence; see also [applying.md](docs/licensing/applying.md) for the (partially complete) checklist of licensing housekeeping beyond the `LICENSE` file itself — SPDX headers, a formal `NOTICE`, `TRADEMARKS.md` and DCO sign-off are not yet done
- [Self-hosting on Gitea](docs/self-hosting-gitea.md)
- [Cross-project notes](docs/cross-project-notes.md): Raspberry Pi companion-project detail carried over from before this repo split off from it — background only; that project is separate and still private, so none of it can be verified from here

## Secrets

Each target's `src/config.h` holds WiFi passwords and a Telegram bot token. All three targets (repo root and the two boards under `boards/`) follow the same pattern: copy that target's own `src/config.example.h` to `src/config.h` and fill in your own values. `.gitignore` excludes every target's `src/config.h`, so it never gets committed. Full walkthrough: [docs/easy-setup/README.md](docs/easy-setup/README.md#setting-up-your-own-secrets-do-this-after-installing-the-software).

## Support this project

This is a free, hobby project — nothing here is paywalled, and it'll stay
that way. If it's saved you some time or you just want to say thanks,
[ko-fi.com/vitoi](https://ko-fi.com/vitoi) is there, no obligation either way.

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE).
