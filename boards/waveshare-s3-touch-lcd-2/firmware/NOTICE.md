# Third-party acknowledgements — Waveshare ESP32-S3-Touch-LCD-2 board

This firmware's code is original: written from scratch, or generated with AI
assistance directed by the project owner, from facts gathered by reading
Waveshare's own published documentation and official demo code for this exact
board (see `../board_notes.md`). No third-party source file is compiled into
this firmware.

Pin numbers, I2C addresses and chip identifications were read from Waveshare's
own wiki pages and the official demo archive for this board, as good practice,
and are credited here even though facts of this kind (what pin does what,
what address a chip answers on) are not protected by copyright — see
`docs/licensing/options.md` (the "caveat about GPL and PID data" note) in the
repo root for the fuller reasoning:

- **Waveshare ESP32-S3-Touch-LCD-2 wiki/documentation** — overview, Arduino
  setup and resources pages. These pages carry no pin tables themselves (see
  `../board_notes.md`, which notes "the wiki pages contain NO pin tables");
  they only confirm general board facts.
  https://docs.waveshare.com/ESP32-S3-Touch-LCD-2
  https://docs.waveshare.com/ESP32-S3-Touch-LCD-2/Arduino
  https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2
  https://docs.waveshare.com/ESP32-S3-Touch-LCD-2/Resources-And-Documents
- **Waveshare's official demo archive for this board**
  (`ESP32-S3-Touch-LCD-2-Demo.zip`) — the actual source, per `../board_notes.md`
  §1-2, of every pin number, I2C address and library choice used here: the
  display SPI pins, the CST816 touch controller's I2C address (0x15), the
  QMI8658 IMU's I2C address (0x6B), the SD-over-SPI pins, the battery ADC pin,
  and the lvgl / Arduino_GFX (`GFX_Library_for_Arduino`) / FastIMU library
  versions the demo pairs with this hardware. This project's own drivers
  (`src/board/board.cpp`, `src/input/touch.cpp`, `src/sensors/imu.cpp`) are
  independent implementations — none of the demo's own source files are
  copied — but the pin map and chip addresses they rely on come from this
  archive, as `src/board/board.h` notes directly ("Waveshare ESP32-S3-Touch-LCD-2
  pins (from Waveshare's demo source; see ../../board_notes.md)").
  https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2/ESP32-S3-Touch-LCD-2-Demo.zip

No explicit licence statement was found on the wiki pages or in the demo
archive for this board while writing `../board_notes.md` (unlike the CYD
board's sources, which name MIT or GPL-3.0 licences) — treat the demo as
Waveshare's own example code for use with their hardware, provided without a
stated licence.

The fetched wiki pages and the unpacked demo archive (used for reference
during bring-up) are not part of this firmware and are not distributed with
it.
