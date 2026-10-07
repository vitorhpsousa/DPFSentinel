# NOTICE

This repository's own code is licensed GPL-3.0-or-later (see
[LICENSE](LICENSE)). The firmware also compiles against the third-party
libraries below, each under its own licence — none of this changes the
licence of the code written for this project.

For hardware bring-up facts (pin numbers, register addresses) credited to
other people's public work, see each touchscreen board's own acknowledgements
file: [boards/cyd-s3-3p5/firmware/NOTICE.md](boards/cyd-s3-3p5/firmware/NOTICE.md) and
[boards/waveshare-s3-touch-lcd-2/firmware/NOTICE.md](boards/waveshare-s3-touch-lcd-2/firmware/NOTICE.md).
This page is about compiled library dependencies instead. See
[docs/licensing/dependency-compatibility.md](docs/licensing/dependency-compatibility.md)
for the full GPL-compatibility reasoning.

## Libraries compiled into the firmware

- **NimBLE-Arduino 1.4.3** (h2zero) — Apache License 2.0. Used by all three
  targets (repo root, `boards/cyd-s3-3p5/`, `boards/waveshare-s3-touch-lcd-2/`)
  for the BLE client. [Verified]: the installed package's own `LICENSE` file
  is the Apache-2.0 text.
  https://github.com/h2zero/NimBLE-Arduino
- **Adafruit GFX Library 1.11.9** — BSD License ("Software License
  Agreement (BSD License), Copyright (c) 2012 Adafruit Industries"). Used by
  the CYD 3.5" board's display driver. [Verified]: the installed package's
  own `license.txt` file.
  https://github.com/adafruit/Adafruit-GFX-Library
- **GFX Library for Arduino 1.5.0** (moononournation, "Arduino_GFX") —
  Apache License 2.0 per the library's own GitHub repository.
  [Unverified from the installed package]: this version does not ship its
  own `LICENSE` file or a licence field in `library.properties`, so this
  was not independently re-checked from the installed copy. Used by the
  Waveshare 2" board's display driver.
  https://github.com/moononournation/Arduino_GFX
- **Arduino-ESP32 core** (`framework = arduino`, `platform = espressif32`;
  version 3.3.12 as installed for this project) — LGPL-2.1-or-later per the
  project's own GitHub repository; ESP-IDF underneath is Apache-2.0.
  [Unverified from the installed package]: no top-level `LICENSE` file
  ships with the installed framework package — only individual bundled
  components (for example its `BLE` and `libb64` helpers) carry their own.
  Used by all three targets for the Arduino framework itself (WiFi, the web
  server, LittleFS, `Preferences`, etc.).
  https://github.com/espressif/arduino-esp32

## If you ever distribute a compiled `.bin`

Right now this project is source-only — anyone using it builds their own
firmware from source with their own secrets, so there is no compiled binary
to attach obligations to. If that ever changes, the licence terms above
travel with the binary: NimBLE-Arduino, GFX Library for Arduino and the
ESP-IDF portion of the Arduino-ESP32 core are Apache-2.0 (attribution
required, no further obligation); Adafruit GFX Library is BSD (attribution
required); the Arduino-ESP32 core itself is LGPL-2.1-or-later, which
requires making the core's own source (or your changes to it) available and
providing a way to relink it — distributing the stock, unmodified core as
installed by PlatformIO satisfies this.
