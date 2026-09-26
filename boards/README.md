# Boards

Firmware for display boards that run the DPF logger. Each has its own `firmware/` (PlatformIO), `board_notes.md` (pins and what was verified) and `tools/` (small hardware probes).

| Board | Screen | Notes |
|---|---|---|
| `cyd-s3-3p5` | 3.5" 320x480 ST77922 (QSPI), capacitive touch, speaker | Main in-car board. Landscape UI, SD (4-bit), BLE OBD adapter, Telegram, speaker alerts. Uses the PlatformIO platform `espressif32@7.1.3` (Arduino-ESP32 2.x): NimBLE-Arduino 1.4.3 crashes on the 3.x core. |
| `waveshare-s3-touch-lcd-2` | 2" 240x320 ST7789, touch, IMU | Earlier bring-up; touch swipes unreliable (tap the screen sides to change page). |

Before building: copy `src/config.example.h` to `src/config.h` and fill in your WiFi and Telegram bot details. `src/config.h` is gitignored and must never be committed.

Pin maps are from the factory firmware and vendor code, each marked verified or unverified in the notes. Vendor demo sources and the factory flash images are not included.
