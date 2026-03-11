/**
 * pins.h — Guition ESP32-S3-4848S040 hardware pin map
 *
 * Display:   ST7701S, 480×480, 16-bit RGB parallel
 * Touch:     GT911, I2C
 * Flash:     16MB
 * PSRAM:     8MB OPI
 */
#pragma once

// ── RGB parallel display ──────────────────────────────────────────────
#define PIN_RGB_DE      39
#define PIN_RGB_VSYNC   48
#define PIN_RGB_HSYNC   47
#define PIN_RGB_PCLK    18

#define PIN_RGB_R0      17
#define PIN_RGB_R1      16
#define PIN_RGB_R2      21
#define PIN_RGB_R3      11
#define PIN_RGB_R4      10

#define PIN_RGB_G0      12
#define PIN_RGB_G1      13
#define PIN_RGB_G2      14
#define PIN_RGB_G3       0
#define PIN_RGB_G4       9
#define PIN_RGB_G5      46

#define PIN_RGB_B0       4
#define PIN_RGB_B1       5
#define PIN_RGB_B2       6
#define PIN_RGB_B3       7
#define PIN_RGB_B4      15

// ST7701S SPI init bus (shared pins with RGB — init only, then release)
#define PIN_SPI_CS      39
#define PIN_SPI_SCK     48
#define PIN_SPI_MOSI    47

// Backlight
#define PIN_BL          38

// ── GT911 touch (I2C) ─────────────────────────────────────────────────
#define PIN_TOUCH_SDA   19
#define PIN_TOUCH_SCL   45
#define PIN_TOUCH_INT   40
#define PIN_TOUCH_RST   41
#define GT911_ADDR      0x5D

// ── Display dimensions ────────────────────────────────────────────────
#define DISPLAY_W       480
#define DISPLAY_H       480
