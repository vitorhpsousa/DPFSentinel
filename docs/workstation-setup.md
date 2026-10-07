# Workstation setup (Windows, Linux, macOS)

This page covers the computer you work from: installing PlatformIO, flashing the ESP32-S3, writing the Raspberry Pi's SD card and reaching the Pi over SSH with keys. Project-side steps are in [esp32-s3.md](esp32-s3.md) and [pi-setup.md](pi-setup.md).

**How this page was made.** It was written on 2026-09-23 from the official documents listed under "Sources". Commands appear only if they are given in one of those sources. Lines marked **(unverified)** come from general knowledge or from the project, not from a source I read, and have not been run on this project. Nothing here was executed on Windows or Linux; on macOS nothing was executed either.

Assumptions: an ESP32-S3 board with a USB port, a Raspberry Pi 4 and a microSD card. Replace `<pi-user>`, `<pi-host>` and `<pi-ip>` with your own values. Never commit private keys or Wi-Fi passwords.

## 0. Choose your OS

| Task | Windows | Linux | macOS |
|---|---|---|---|
| Install PlatformIO | [W1](#w1-install-platformio) | [L1](#l1-install-platformio) | [M1](#m1-install-platformio) |
| Serial port and permissions | [W2](#w2-usb-serial-port) | [L2](#l2-usb-serial-permissions) | [M2](#m2-usb-serial-port) |
| Flash the S3 | [W3](#w3-flash-the-s3) | [L3](#l3-flash-the-s3) | [M3](#m3-flash-the-s3) |
| Image the Pi | [W4](#w4-image-the-pi) | [L4](#l4-image-the-pi) | [M4](#m4-image-the-pi) |
| SSH with keys | [W5](#w5-ssh-with-keys) | [L5](#l5-ssh-with-keys) | [M5](#m5-ssh-with-keys) |
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

### W4. Image the Pi

Raspberry Pi Imager is available for Windows; download the installer from `https://www.raspberrypi.com/software/` and run it. In the customisation screens set a hostname (reachable as `<hostname>.local`), a username and password, Wi-Fi details and the SSH option. For SSH choose public-key authentication and supply your public key (Raspberry Pi documentation). Note the docs describe supplying an RSA public key file in this dialog; whether Imager accepts an Ed25519 key is **unverified**, so generating an RSA key (below) is the safe option for this step.

### W5. SSH with keys

Windows 10 (build 1809) and later have an OpenSSH client as an optional feature. Check and install from an administrator PowerShell (Microsoft Learn):

```powershell
Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*'
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0
```

Create a key pair (Microsoft's example uses ECDSA; without `-t` it defaults to Ed25519):

```powershell
ssh-keygen -t ecdsa
```

The public key is `id_ecdsa.pub` in `C:\Users\<you>\.ssh\`; the private file has no extension and must stay secret. To keep the key in the agent:

```powershell
Get-Service ssh-agent | Set-Service -StartupType Automatic
Start-Service ssh-agent
ssh-add $env:USERPROFILE\.ssh\id_ecdsa
```

Connect with `ssh <pi-user>@<pi-ip>` or `ssh <pi-user>@<pi-host>.local` (Raspberry Pi docs). Windows has no `ssh-copy-id`; if you did not set your key in Imager, the Raspberry Pi docs describe a manual copy with `scp` and `chmod` steps written for Linux/macOS shells, which I have not adapted for PowerShell (**unverified**). The simplest route is to paste the public key into Imager's SSH step.

**WSL alternative.** The Raspberry Pi docs say the same SSH commands work in PowerShell or WSL. If you flash from WSL 2, USB serial devices are not visible until attached with usbipd-win (Microsoft Learn): install it (`winget install --interactive --exact dorssel.usbipd-win`), then from an administrator PowerShell run `usbipd list`, `usbipd bind --busid <busid>`, then `usbipd attach --wsl --busid <busid>`, and check with `lsusb` inside WSL. While attached, Windows cannot use the device. You may still need the udev/group steps from the Linux section inside WSL. Flashing from native Windows is simpler.

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

### L4. Image the Pi

Raspberry Pi Imager on Linux: on Raspberry Pi OS `sudo apt install rpi-imager`; on other distributions download the AppImage from the Raspberry Pi site and make it executable (Raspberry Pi docs). Use the customisation screens as in W4: hostname, user, Wi-Fi, SSH with your public key.

### L5. SSH with keys

Generate and install a key (Raspberry Pi docs):

```bash
ssh-keygen
ssh-copy-id <pi-user>@<pi-ip>
ssh <pi-user>@<pi-ip>
```

The docs' default key path is `~/.ssh/id_rsa` and the private file must stay on your machine. If `ssh-copy-id` is missing, the docs give a manual method: on the Pi `mkdir .ssh` and `chmod 700 .ssh`, from your computer `scp .ssh/id_rsa.pub <pi-user>@<pi-ip>:.ssh/authorized_keys`, then on the Pi `chmod 644 .ssh/authorized_keys` (this overwrites any existing `authorized_keys`).

Reaching `<pi-host>.local` needs mDNS on your workstation (**how to enable it per distribution is not covered by my sources**); otherwise use the IP shown by your router.

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

### M4. Image the Pi

Download Raspberry Pi Imager for macOS from `https://www.raspberrypi.com/software/`, run it, and use the customisation screens as in W4.

### M5. SSH with keys

macOS ships an OpenSSH client (**unverified** by a source I read, but the Raspberry Pi docs' commands are written for a Unix shell). Use the same commands as L5. `ssh-copy-id` may be absent on macOS; use the manual `scp` method from L5 if so (**unverified** whether it is present on your version). `<pi-host>.local` resolves via macOS's built-in mDNS (**unverified** by a source).

## Common errors

| Symptom | Likely cause and documented fix |
|---|---|
| Permission denied on the serial port (Linux) | Not in `dialout` (or `uucp`), or no udev rules. See L2; log out and in afterwards. |
| Port not listed | Bad or charge-only cable (general advice, unverified), missing driver on Windows for boards with a USB-serial chip, or the S3 stuck. Try manual download mode: GPIO0 low, then reset (Espressif). |
| "Failed to connect" / no bootloader reply | Check the correct port, stable 3.3 V power, and that boot-mode pins are as expected; esptool suggests trying a lower baud such as `--baud 9600` and disconnecting devices from GPIO pins (esptool troubleshooting). Under PlatformIO the baud option name is **unverified**. |
| Flashes but does not run | esptool suggests a power supply that can deliver enough current, and DIO flash mode for some devices (`esptool write_flash -fm dio`). For PlatformIO the equivalent setting is **unverified**. |
| WSL cannot see the board | Attach it with usbipd-win (W5) and close serial monitors on the Windows side. |
| SSH refuses key or asks for password | Public key not in `~/.ssh/authorized_keys` on the Pi, or wrong permissions. See L5. |
| `.local` name not found | mDNS not available; use the Pi's IP address (**general advice**). |
| Serial monitor restarts the board | Expected. Opening the port resets the S3 and starts a new session (see [esp32-s3.md](esp32-s3.md)). |
| Bluetooth adapter questions (Pi) | Not a workstation matter; see [pi-setup.md](pi-setup.md). |

## Sources (all checked 2026-09-23)

- PlatformIO Core install methods: https://docs.platformio.org/en/latest/core/installation/index.html
- PlatformIO installer script: https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html
- PlatformIO shell commands (PATH): https://docs.platformio.org/en/latest/core/installation/shell-commands.html
- PlatformIO udev rules: https://docs.platformio.org/en/latest/core/installation/udev-rules.html
- PlatformIO Espressif 32 platform: https://docs.platformio.org/en/latest/platforms/espressif32.html
- Espressif, ESP32-S3 USB Serial/JTAG: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html
- esptool troubleshooting: https://docs.espressif.com/projects/esptool/en/latest/esp32s3/troubleshooting.html
- esptool serial connection: https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/serial-connection.html
- Raspberry Pi getting started (Imager): https://www.raspberrypi.com/documentation/computers/getting-started.html
- Raspberry Pi remote access (SSH): https://www.raspberrypi.com/documentation/computers/remote-access.html
- Microsoft, OpenSSH install: https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_install_firstuse
- Microsoft, OpenSSH key management: https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_keymanagement
- Microsoft, USB devices in WSL: https://learn.microsoft.com/en-us/windows/wsl/connect-usb

The page contents were read through a summarising fetch tool, so re-check any command against the source before relying on it, and re-check dates when versions change.
