# Workstation setup (Windows, Linux, macOS)

> **Scope note.** This repo holds the ESP32-S3 firmware only. This page covers
> installing PlatformIO and flashing the ESP32-S3, per OS. It used to also
> cover imaging a Raspberry Pi's SD card and reaching it over SSH, from when
> the ESP32 and Pi loggers shared one project; that Pi-specific imaging/SSH
> material has moved to
> [cross-project-notes.md](cross-project-notes.md#workstation-setup-imaging-and-ssh-ing-into-the-pi)
> for anyone setting up both projects together. The Pi firmware/app itself
> lives in a separate, currently-private repo (`obd-pi`), not in this one.

This page covers the computer you work from: installing PlatformIO and flashing the ESP32-S3. Project-side steps for the ESP32-S3 are in [esp32-s3.md](esp32-s3.md); the Pi's own setup walkthrough (`pi-setup.md`) lives in the separate `obd-pi` repo and isn't available here.

**How this page was made.** It was written on 2026-09-23 from the official documents listed under "Sources". Commands appear only if they are given in one of those sources. Lines marked **(unverified)** come from general knowledge or from the project, not from a source I read, and have not been run on this project. Nothing here was executed on Windows or Linux; on macOS nothing was executed either.

Assumptions: an ESP32-S3 board with a USB port. Never commit private keys or Wi-Fi passwords.

## 0. Choose your OS

| Task | Windows | Linux | macOS |
|---|---|---|---|
| Install PlatformIO | [W1](#w1-install-platformio) | [L1](#l1-install-platformio) | [M1](#m1-install-platformio) |
| Serial port and permissions | [W2](#w2-usb-serial-port) | [L2](#l2-usb-serial-permissions) | [M2](#m2-usb-serial-port) |
| Flash the S3 | [W3](#w3-flash-the-s3) | [L3](#l3-flash-the-s3) | [M3](#m3-flash-the-s3) |
| Errors | [Common errors](#common-errors) | | |

Things the same on every OS:

- The board's USB port appears as a serial port: `COM<n>` on Windows, `/dev/ttyACM*` on Linux, `/dev/cu*` on macOS, when the S3's built-in USB Serial/JTAG is used (Espressif). Boards with a separate USB-to-serial chip may show up under different names.
- The S3 can normally be put into download mode by the flasher automatically. If the board is unresponsive, hold GPIO0 low (the BOOT button on most boards, **unverified** for your board) and reset it (Espressif).
- The flash and monitor commands are the same everywhere: from the `obd-esp32` repo root, `pio run -t upload` and `pio device monitor` (from [esp32-s3.md](esp32-s3.md); I did not find them in the PlatformIO pages I fetched, so they are **unverified** here). Opening the monitor resets the board and starts a new logging session.

## Windows

### W1. Install PlatformIO

PlatformIO's docs recommend the installer script. Download `get-platformio.py` from `https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py`, then in PowerShell:

```powershell
cd C:/path-to-dir/where/get-platformio.py/is-located
python.exe get-platformio.py
```

This needs Python installed (**how to install it is not covered by the sources I read**). To use `pio` from any terminal, add `%USERPROFILE%\.platformio\penv\Scripts\` to the start of the system `Path` variable (PlatformIO shell-commands page), then open a new terminal.

An alternative is the PlatformIO IDE extension for VS Code, which bundles Core so no separate install is needed (PlatformIO docs say Core is not required for the IDE). The extension steps are **unverified** here.

### W2. USB serial port

The S3's native USB appears as a `COM` port in Device Manager and uses standard drivers (Espressif; the page names no extra driver). If your board has a separate USB-serial chip, install the driver from the board or chip maker (PlatformIO's ESP32 page: check for a correctly installed USB driver from the board maker). Which chip your board has is **unknown until you read its documentation**.

### W3. Flash the S3

```powershell
pio run -t upload
pio device monitor
```

(**unverified** on Windows; commands as in the project docs; run from the `obd-esp32` repo root). If the port is not found, check Device Manager for the COM number and try a different cable (many cables are power-only, **unverified** general advice).

**Flashing from WSL.** USB serial devices are not visible inside WSL 2 until attached with usbipd-win (Microsoft Learn): install it (`winget install --interactive --exact dorssel.usbipd-win`), then from an administrator PowerShell run `usbipd list`, `usbipd bind --busid <busid>`, then `usbipd attach --wsl --busid <busid>`, and check with `lsusb` inside WSL. While attached, Windows cannot use the device. You may still need the udev/group steps from the Linux section inside WSL. Flashing from native Windows is simpler.

Imaging a Raspberry Pi's SD card and SSH-ing into it are not covered here — see [cross-project-notes.md](cross-project-notes.md#workstation-setup-imaging-and-ssh-ing-into-the-pi) if you also have the separate `obd-pi` project.

## Linux

### L1. Install PlatformIO

Installer script (PlatformIO docs):

```bash
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

For `pio` on your PATH, link the binaries and make sure `~/.local/bin` is on PATH (PlatformIO shell-commands page):

```bash
ln -s ~/.platformio/penv/bin/platformio ~/.local/bin/platformio
ln -s ~/.platformio/penv/bin/pio ~/.local/bin/pio
ln -s ~/.platformio/penv/bin/piodebuggdb ~/.local/bin/piodebuggdb
export PATH=$PATH:$HOME/.local/bin     # put this line in ~/.profile
```

### L2. USB serial permissions

Without permission you get "permission denied" on the port. Two documented fixes; pick one.

Option A, udev rules from PlatformIO (recommended by PlatformIO):

```bash
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

then unplug and replug the board. (PlatformIO also lists `sudo service udev restart` as an alternative.)

Option B, group membership (esptool docs and PlatformIO docs agree on `dialout`; Arch-style distributions use `uucp`):

```bash
sudo usermod -a -G dialout $USER
```

Log out and back in (or reboot), then run `id` to confirm the group is listed. Do not work around this by running `pio` with `sudo` (**general advice, unverified**; it can leave root-owned files in your project and `~/.platformio`).

### L3. Flash the S3

```bash
pio run -t upload
pio device monitor
```

(**unverified** on Linux; run from the `obd-esp32` repo root). The port is usually `/dev/ttyACM0` for the native USB (Espressif names `/dev/ttyACM*`). Some desktop services such as ModemManager or brltty are known to grab serial devices; that is common community advice, not from a source I read (**unverified**).

## macOS

### M1. Install PlatformIO

The docs list the installer script, pip, and Homebrew. The installer script:

```bash
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

Then the PATH links as in L1 (the shell-commands page covers Unix-like systems including macOS). The Homebrew formula name and command (`brew install platformio`) are **unverified**; the PlatformIO page only names Homebrew as an option.

### M2. USB serial port

The S3's native USB shows up as `/dev/cu*` (Espressif). No Linux-style group step is needed for this (**unverified**; I did not find macOS permission notes in the sources). Boards with a separate USB-serial chip may need a chip-maker driver; check your board's documentation (**unknown**).

### M3. Flash the S3

```bash
pio run -t upload
pio device monitor
```

(**unverified** on macOS, though this project's author uses a Mac; run from the `obd-esp32` repo root). If several ports exist, `pio device list` lists them (**unverified**).

## Common errors

| Symptom | Likely cause and documented fix |
|---|---|
| Permission denied on the serial port (Linux) | Not in `dialout` (or `uucp`), or no udev rules. See L2; log out and in afterwards. |
| Port not listed | Bad or charge-only cable (general advice, unverified), missing driver on Windows for boards with a USB-serial chip, or the S3 stuck. Try manual download mode: GPIO0 low, then reset (Espressif). |
| "Failed to connect" / no bootloader reply | Check the correct port, stable 3.3 V power, and that boot-mode pins are as expected; esptool suggests trying a lower baud such as `--baud 9600` and disconnecting devices from GPIO pins (esptool troubleshooting). Under PlatformIO the baud option name is **unverified**. |
| Flashes but does not run | esptool suggests a power supply that can deliver enough current, and DIO flash mode for some devices (`esptool write_flash -fm dio`). For PlatformIO the equivalent setting is **unverified**. |
| WSL cannot see the board | Attach it with usbipd-win (see W3) and close serial monitors on the Windows side. |
| Serial monitor restarts the board | Expected. Opening the port resets the S3 and starts a new session (see [esp32-s3.md](esp32-s3.md)). |

## Sources (all checked 2026-09-23)

- PlatformIO Core install methods: https://docs.platformio.org/en/latest/core/installation/index.html
- PlatformIO installer script: https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html
- PlatformIO shell commands (PATH): https://docs.platformio.org/en/latest/core/installation/shell-commands.html
- PlatformIO udev rules: https://docs.platformio.org/en/latest/core/installation/udev-rules.html
- PlatformIO Espressif 32 platform: https://docs.platformio.org/en/latest/platforms/espressif32.html
- Espressif, ESP32-S3 USB Serial/JTAG: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html
- esptool troubleshooting: https://docs.espressif.com/projects/esptool/en/latest/esp32s3/troubleshooting.html
- esptool serial connection: https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/serial-connection.html
- Microsoft, USB devices in WSL: https://learn.microsoft.com/en-us/windows/wsl/connect-usb

(Sources for imaging a Raspberry Pi's SD card and SSH-ing into it moved with
that content to [cross-project-notes.md](cross-project-notes.md#workstation-setup-imaging-and-ssh-ing-into-the-pi).)

The page contents were read through a summarising fetch tool, so re-check any command against the source before relying on it, and re-check dates when versions change.
