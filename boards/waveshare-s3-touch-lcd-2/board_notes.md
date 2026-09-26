# Waveshare ESP32-S3-Touch-LCD-2 board notes

Sources (fetched):
- W1 https://docs.waveshare.com/ESP32-S3-Touch-LCD-2 (overview)
- W2 https://docs.waveshare.com/ESP32-S3-Touch-LCD-2/Arduino
- W3 https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2
- W4 https://docs.waveshare.com/ESP32-S3-Touch-LCD-2/Resources-And-Documents
- D  Official demo zip https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2/ESP32-S3-Touch-LCD-2-Demo.zip
  (downloaded and unpacked in session scratchpad; file paths below are inside it)

The wiki pages contain NO pin tables (W1/W3 say so). All pins below come from the
official demo source (D). The schematic PDF (https://files.waveshare.com/wiki/ESP32-S3-LCD-2/ESP32-S3-Touch-LCD-2-SchDoc.pdf)
could not be text-read, so nothing is schematic-verified.

## 1. Pin map (source: D unless noted)

| Function | GPIO | Where in D |
|---|---|---|
| LCD SCLK | 39 | Arduino/examples/01_factory/bsp_lv_port.h |
| LCD MOSI | 38 | same |
| LCD MISO | 40 (shared bus, used by SD) | same |
| LCD DC | 42 | same |
| LCD CS | 45 | same |
| LCD RST | -1 (none / not driven by a GPIO) | same |
| LCD backlight | 1 (LEDC PWM 5 kHz, 10 bit) | Arduino/examples/06_lvgl_battery/06_lvgl_battery.ino |
| LCD pixel clock | 80 MHz | bsp_lv_port.h |
| I2C SDA (touch + IMU) | 48 | 01_factory/bsp_i2c.h, 04_qmi8658_output.ino (Wire.begin(48,47)) |
| I2C SCL (touch + IMU) | 47 | same |
| Touch CST816 I2C addr | 0x15 | Arduino/libraries/bsp_cst816 header |
| Touch RST | -1 (none) | bsp_cst816 header (EXAMPLE_PIN_NUM_TP_RST) |
| Touch INT | NOT VERIFIED (demo polls I2C; no INT pin found) | - |
| IMU QMI8658 | I2C, addr 0x6B, same SDA48/SCL47 bus | 04_qmi8658_output.ino |
| SD card | SPI (not SDMMC): SCK 39, MOSI 38, MISO 40, CS 41 | 03_sd_card_test.ino, ESP-IDF/01_sd_card_test/main/sd_card_example_main.c |
| Battery ADC | GPIO 5, voltage = 3.3/4096*raw*3 (i.e. 1:3 divider assumed by demo) | 06_lvgl_battery.ino |
| Camera (OV5640 24-pin FPC) | XCLK 8, SIOD 21, SIOC 16, VSYNC 6, HREF 4, PCLK 9, Y9..Y2 = 2,7,10,14,11,15,13,12 | W3 (demo code excerpt) |
| I/O expander | None found in demo code (NOT VERIFIED against schematic) | grep of examples |
| Buzzer | None found in demo code / W1 (NOT VERIFIED; probably absent) | - |
| RTC | None found in demo code / W1 | - |
| Free/expansion header pins | NOT VERIFIED. W1 says "22 pins available" only; no header pin list retrieved. Note camera FPC uses GPIO 2,4,6,7,8,9,10-16,21, so a free GPIO list must be read from the schematic PDF. | W1 |

Flags: GPIO 1 pinned to backlight; SD and LCD share SPI bus (39/38/40) with separate CS (41 vs 45).
Conflict to check: W3 camera SIOD=21/SIOC=16 differ from the display I2C (48/47), so camera has its own SCCB bus.

## 2. Arduino / build settings

- Board: "ESP32S3 Dev Module", arduino-esp32 core >= 3.0.0, USB CDC must be enabled (W3). Partition scheme not specified on wiki (W3).
- Libraries (W2/W3): lvgl 8.4.0, GFX_Library_for_Arduino 1.5.0 (Arduino_GFX, ST7789 over SPI), FastIMU 1.2.6 on the wiki (zip bundles 1.2.8), bsp_cst816 (offline, bundled in D/Arduino/libraries). Wiki warns LVGL and driver versions are tightly coupled.
- lv_conf.h: LV_COLOR_DEPTH 16, LV_MEM_CUSTOM 0 (D 01_factory/lv_conf.h).
- Demo FQBN hint (D 10_camera_web_server/ci.json): `esp32s3:PSRAM=opi,USBMode=default,PartitionScheme=custom,FlashMode=qio`.
- ESP-IDF sdkconfig (D ESP-IDF/06_lvgl_example/sdkconfig): flash 16MB, QIO flash mode set (FLASHMODE_QIO=y; a stray FLASHMODE="dio" string also present), PSRAM enabled octal mode (SPIRAM_MODE_OCT) at 80 MHz, IDF console not on USB CDC (UART default), partition table custom "partitions.csv".
- Camera example partition (D 10_camera_web_server/partitions.csv): nvs, otadata, app0 0x3c0000 at 0x10000, fr, coredump (only ~4MB used of 16MB).
- PlatformIO suggestion (derived, untested): board_build.arduino.memory_type = qio_opi, flash_mode qio, board_upload.flash_size 16MB, ARDUINO_USB_CDC_ON_BOOT=1, custom or default_16MB.csv partitions. Not from Waveshare.

## 3. What helps a car DPF logger (honest)

- Touch 240x320 IPS LCD + LVGL: soot/regen dashboard, page switching. Usable in daylight only moderately; small.
- QMI8658 IMU (I2C 0x6B): harsh braking/accel and road vibration markers, tilt. Board orientation must be fixed in mount.
- Battery ADC GPIO5: reads the LiPo (demo 1:3 ratio assumption; calibrate). It does NOT measure the 12 V car rail; use an external divider on a free GPIO if wanted (free pins NOT VERIFIED).
- TF slot (SPI): CSV logging, FAT32 required (W2). Shared SPI bus with LCD, so log in batches.
- BLE 5 + Wi-Fi: talk to BLE OBD adapter, upload logs at home.
- 8MB PSRAM, 16MB flash: plenty for buffers/history.
- MX1.25 LiPo header with charger: survives ignition-off to flush logs.
Limits:
- No RTC on this board (none found; wiki lists none): need external DS3231 on I2C 47/48 or NTP/OBD time.
- No buzzer found (not verified).
- Heat: LiPo in a hot cabin is a risk (general knowledge, not from Waveshare); no operating temp range retrieved from docs (NOT VERIFIED).
- Touch and IMU share I2C with no INT pin known, so polling only.
- BLE + Wi-Fi share one radio; LVGL + BLE needs care with RAM.
