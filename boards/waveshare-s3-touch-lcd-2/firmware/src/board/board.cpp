#include "board.h"

static SPIClass s_spi(FSPI);
static Arduino_DataBus *s_bus = nullptr;
static Arduino_GFX *s_gfx = nullptr;

SPIClass &boardSpi() { return s_spi; }
Arduino_GFX *boardGfx() { return s_gfx; }

bool boardInit() {
    s_spi.begin(BOARD_PIN_SCLK, BOARD_PIN_MISO, BOARD_PIN_MOSI, -1);  // CS pins are driven per device
    pinMode(BOARD_PIN_SD_CS, OUTPUT);
    digitalWrite(BOARD_PIN_SD_CS, HIGH);  // keep the card deselected while the panel talks
    s_bus = new Arduino_HWSPI(BOARD_PIN_LCD_DC, BOARD_PIN_LCD_CS, BOARD_PIN_SCLK, BOARD_PIN_MOSI, BOARD_PIN_MISO, &s_spi);
    s_gfx = new Arduino_ST7789(s_bus, -1 /* no RST pin */, 0 /* portrait */, true /* IPS */, 240, 320);
    pinMode(BOARD_PIN_BL, OUTPUT);
    digitalWrite(BOARD_PIN_BL, HIGH);
    return s_gfx->begin(40000000);
}
