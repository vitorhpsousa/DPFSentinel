// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 the DPF Sentinel project
#pragma once
// Waveshare ESP32-S3-Touch-LCD-2 pins (from Waveshare's demo source; see ../../board_notes.md).
#include <Arduino.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>

#define BOARD_PIN_SCLK 39
#define BOARD_PIN_MOSI 38
#define BOARD_PIN_MISO 40
#define BOARD_PIN_LCD_DC 42
#define BOARD_PIN_LCD_CS 45
#define BOARD_PIN_SD_CS 41
#define BOARD_PIN_BL 1
#define BOARD_PIN_SDA 48
#define BOARD_PIN_SCL 47
#define BOARD_PIN_BATT 5

// The LCD and the SD card share one SPI bus (separate chip selects), so both
// must use this SPIClass. The display is 240x320 portrait after boardInit().
SPIClass &boardSpi();
// Starts the shared SPI bus + backlight + display (rotation 0, 240x320). Returns false if the panel fails.
bool boardInit();
Arduino_GFX *boardGfx();
