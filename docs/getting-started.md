# Getting started

This repo holds three separate ESP32-S3 firmware targets, each its own PlatformIO
project. This page is the front door: what software you need on your workstation,
and an ordered checklist to get each target building and flashed. It does not
repeat the deep-dive docs — follow the links for wiring, pin maps, protocol
detail and known issues.

## Software prerequisites (all targets)

- **PlatformIO** — either the `pio` CLI (PlatformIO Core) or VS Code with the
  PlatformIO IDE extension. Install steps per OS, serial port permissions and
  common flashing errors are in [workstation-setup.md](workstation-setup.md).
- **git**, to clone this repo.
- **A USB cable that actually carries data.** Many cables are charge-only and
  will not show up as a serial port; this trips people up often enough to call
  out here.
- **A Telegram bot token and chat id**, only if you want Telegram alerts on any
  target. Get them from `@BotFather` and set them up following
  [telegram-alerts.md](telegram-alerts.md) — don't duplicate those steps here.
- **A microSD card plus a card reader on your workstation**, only for the two
  touchscreen boards (targets 2 and 3 below). Not needed for target 1, which
  uses internal flash by default.

## Target 1: original ESP32-S3 BLE logger (repo root)

The original build: an ESP32-S3 talking to a BLE ELM327 adapter, logging to
microSD or internal flash, with a web UI and Telegram alerts. Lives at the
**repo root** (`platformio.ini`, `src/main.cpp`, `src/obd/`, etc.) — there is
no subfolder for it. Full detail: [esp32-s3.md](esp32-s3.md).

1. From the repo root, edit `src/config.h` directly with your real WiFi and
   Telegram values (`WIFI_AP_PASS`, `TG_BOT_TOKEN`, `TG_CHAT_ID`, and whatever
   else you need — see the settings table in [esp32-s3.md](esp32-s3.md)).
   **This file is tracked by git**, unlike the two boards below. There is no
   `config.example.h` for this target and no gitignore protecting it. Once you
   put real secrets in it, do not `git add` or commit it — check `git status`
   before committing anything at the repo root.
2. Build and flash from the repo root:
   ```bash
   pio run -t upload
   pio device monitor
   ```
   Opening the monitor resets the board and starts a new log session.
3. First connection, Telegram setup, web UI paths and known issues are all in
   [esp32-s3.md](esp32-s3.md) and [telegram-alerts.md](telegram-alerts.md).

## Target 2: CYD 3.5" board (main in-car board)

`boards/cyd-s3-3p5/firmware/`. 320x480 ST77922 touchscreen (QSPI), capacitive
touch, speaker, SD card. Board-specific detail: [boards/README.md](../boards/README.md)
and [boards/cyd-s3-3p5/board_notes.md](../boards/cyd-s3-3p5/board_notes.md).

1. **Format your microSD card FAT32 on your workstation first**, with any
   normal formatter (Disk Utility, gparted, Windows Format). The firmware
   never auto-formats a card (`SD_MMC.begin("/sdcard", false, false)`), and a
   freshly-inserted unformatted card silently falls back to internal flash
   (only a few MB free) instead of erroring — you will not get a warning, so
   do this step before first boot.
2. Copy `boards/cyd-s3-3p5/firmware/src/config.example.h` to
   `boards/cyd-s3-3p5/firmware/src/config.h` and fill in your WiFi and
   Telegram details. This file is gitignored (`firmware/.gitignore` lists
   `src/config.h`) — safe to edit in place.
3. Build and flash:
   ```bash
   cd boards/cyd-s3-3p5/firmware
   pio run -t upload
   pio device monitor
   ```
4. **Do not change the platform pin.** `platformio.ini` pins
   `platform = espressif32@7.1.3` deliberately — NimBLE-Arduino 1.4.3 crashes
   with a Guru Meditation panic on Arduino-ESP32 3.x. Never bump this to a
   default or latest platform version.
5. Verify the SD card actually mounted: watch the serial boot line, or once
   the board is on WiFi, hit `http://<board-ip>/api/live` and check the JSON
   field — `"storage":"SD"` means it mounted, `"storage":"flash"` means it
   silently fell back (go back to step 1).

## Target 3: Waveshare 2" board (earlier bring-up)

`boards/waveshare-s3-touch-lcd-2/firmware/`. 240x320 ST7789 screen, touch,
IMU. Board-specific detail: [boards/README.md](../boards/README.md) and
[boards/waveshare-s3-touch-lcd-2/board_notes.md](../boards/waveshare-s3-touch-lcd-2/board_notes.md).

1. Copy `boards/waveshare-s3-touch-lcd-2/firmware/src/config.example.h` to
   `boards/waveshare-s3-touch-lcd-2/firmware/src/config.h` and fill in your
   WiFi and Telegram details. Also gitignored — safe to edit in place.
2. Build and flash:
   ```bash
   cd boards/waveshare-s3-touch-lcd-2/firmware
   pio run -t upload
   pio device monitor
   ```
   Unlike the CYD board, this target's `platformio.ini` uses a plain
   `espressif32` platform with no version pin — no special handling needed
   here.
3. Known quirk: touch swipes are unreliable on this board. Tap near the
   left/right edge of the screen to change pages instead of swiping.

## Where to go next

- Installing PlatformIO per OS, serial port permissions, common flashing
  errors: [workstation-setup.md](workstation-setup.md)
- Full detail on target 1: [esp32-s3.md](esp32-s3.md)
- Board overview and both boards' pin/verification notes:
  [boards/README.md](../boards/README.md)
- Telegram bot setup (any target): [telegram-alerts.md](telegram-alerts.md)
