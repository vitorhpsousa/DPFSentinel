# Manual setup (no AI agent required)

[ai-setup-prompt.md](ai-setup-prompt.md) describes an *optional* interview-style
walkthrough for an AI assistant. This page is the alternative: every step you
need, done by hand, in your own terminal, with commands you read before you run
them. Nothing here needs an agent with shell or hardware access.

## Why do it this way

- **You decide what gets written.** Flashing firmware, formatting an SD card,
  and binding a Bluetooth device are hard-to-reverse actions on your own
  hardware. Doing them yourself means nothing runs against your board or car
  that you did not personally read and approve first.
- **Secrets stay off the record.** WiFi passwords, the Telegram bot token, and
  the adapter's Bluetooth MAC address go straight into local, git-ignored (or
  git-status-checked) files on your machine — never typed into a chat with an
  assistant, and never something an assistant needs to see to help you.
- **Physical steps need a human anyway.** Pressing the adapter's Connect
  button, plugging in a USB cable, reading a label for a MAC address, watching
  a serial monitor — no agent can do these for you, so there is no real
  time saved by involving one for the rest of the flow either.

## The general shape, every target

1. Install the toolchain on your own workstation. See
   [workstation-setup.md](workstation-setup.md) for per-OS PlatformIO, SSH and
   driver steps.
2. Clone the repo (or already have it) and read `git status` before you
   commit anything, especially after editing a config file with real secrets
   in it.
3. Copy the example config for your target (if one exists — see below), fill
   in your own values, and confirm it is git-ignored or that you will not
   stage it.
4. Build and flash with a command you type yourself, in a terminal you are
   watching.
5. Verify with your own eyes: the serial monitor, a web endpoint, or a file
   appearing where you expect it.

## Target 1: ESP32-S3 BLE logger (this repo's root)

The original firmware. `platformio.ini` and `src/` live at this repo's root —
no subfolder.

1. Install PlatformIO ([workstation-setup.md](workstation-setup.md) §W1/L1/M1).
2. Edit `src/config.h` directly with your real WiFi and Telegram values (see
   the settings table in [esp32-s3.md](esp32-s3.md#configuration-srcconfigh)).
   **There is no `config.example.h` for this target and no `.gitignore` entry
   protects it** — it is tracked by git. Once you put real secrets in it, do
   not `git add` or commit it; run `git status` before every commit at this
   repo's root.
3. Build and flash, from the repo root, in your own terminal:
   ```bash
   pio run -t upload
   pio device monitor
   ```
   (OS-specific notes and common errors: [workstation-setup.md](workstation-setup.md) §W3/L3/M3 and
   [Common errors](workstation-setup.md#common-errors).) Opening the monitor
   resets the board and starts a new session.
4. Verify: watch the serial monitor for the BLE connect and adapter-init
   lines. Full detail on first connection and known issues:
   [esp32-s3.md](esp32-s3.md#first-connection).

## Target 2: CYD 3.5" board (`boards/cyd-s3-3p5`)

Main in-car board, 320x480 touchscreen.

1. **Format your microSD card FAT32 on your workstation first**, with any
   normal formatter. The firmware never auto-formats a card, and an
   unformatted card silently falls back to internal flash (a few MB) with no
   warning — do this before first boot.
2. Copy `boards/cyd-s3-3p5/firmware/src/config.example.h` to
   `boards/cyd-s3-3p5/firmware/src/config.h` and fill in your own WiFi and
   Telegram details. This file is gitignored — safe to edit in place.
3. Build and flash:
   ```bash
   cd boards/cyd-s3-3p5/firmware
   pio run -t upload
   pio device monitor
   ```
4. **Do not change the pinned platform.** `platformio.ini` pins
   `platform = espressif32@7.1.3` deliberately — NimBLE-Arduino 1.4.3 crashes
   on Arduino-ESP32 3.x. Leave it alone.
5. Verify the SD card mounted: watch the serial boot line, or once on WiFi
   hit `http://<board-ip>/api/live` and check `"storage":"SD"` (not
   `"flash"` — if it says flash, go back to step 1).

Board notes and pin maps: `boards/README.md` and
`boards/cyd-s3-3p5/board_notes.md`.

## Target 3: Waveshare 2" board (`boards/waveshare-s3-touch-lcd-2`)

Earlier bring-up board, 240x320 screen.

1. Copy `boards/waveshare-s3-touch-lcd-2/firmware/src/config.example.h` to
   `boards/waveshare-s3-touch-lcd-2/firmware/src/config.h` and fill in your
   own WiFi and Telegram details. Also gitignored.
2. Build and flash:
   ```bash
   cd boards/waveshare-s3-touch-lcd-2/firmware
   pio run -t upload
   pio device monitor
   ```
   This target's `platformio.ini` uses a plain `espressif32` platform with no
   version pin — no special handling needed.
3. Known quirk: touch swipes are unreliable on this board. Tap near the
   left/right edge of the screen to change pages instead of swiping.

Board notes: `boards/README.md` and
`boards/waveshare-s3-touch-lcd-2/board_notes.md`.

## Target 4: Raspberry Pi logger (the `obd-pi` repo)

No firmware flash here, but the same principle applies: image the card and
install the service yourself rather than handing an agent your Pi's SSH
session.

1. Image the SD card with Raspberry Pi Imager on your own workstation, using
   its customisation screens to set hostname/user/WiFi/SSH before writing the
   card (`workstation-setup.md` §W4/L4/M4 in the `obd-pi` repo, for imaging
   and SSH-key setup).
2. SSH in yourself and install: `sudo apt install bluez python3-venv`, clone
   the repo, `python3 -m venv venv`, `venv/bin/pip install -r requirements.txt`.
3. Pair and bind the adapter yourself with `bluetoothctl` and
   `scripts/bind_rfcomm.sh` — press the adapter's physical Connect button
   yourself and read its MAC address off the label.
4. Edit `config.py` yourself (at minimum `LOG_DIR`).
5. Run it by hand once (`venv/bin/python3 main.py`) and confirm a
   `session_N.csv` appears before installing the systemd units.

Full walkthrough: `pi-setup.md` in the `obd-pi` repo.

## What to keep out of an agent's hands

- Never paste a real WiFi password, Telegram bot token, or adapter Bluetooth
  address into a chat with an assistant — put it directly in the local config
  file instead.
- Be wary of asking an agent to run `pio run -t upload`, format a card, or
  `rfcomm bind` against your real hardware unless you are watching and would
  type the same command yourself.
- Steps that need physical access (pressing a button, reading a label,
  plugging in a cable, watching a serial monitor) are yours regardless — an
  agent cannot do them, so there's nothing to delegate there in the first
  place.

## See also

- [ai-setup-prompt.md](ai-setup-prompt.md) — the interview-style AI-assisted
  route, if you want a copilot instead of doing it solo
- [workstation-setup.md](workstation-setup.md) — full per-OS toolchain detail
- [esp32-s3.md](esp32-s3.md) — full configuration and firmware detail
- `boards/README.md` — board-specific pin maps and notes
- `pi-setup.md` (in the `obd-pi` repo) — full Pi walkthrough
