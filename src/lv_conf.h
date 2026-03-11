/**
 * lv_conf.h — LVGL v8 configuration for Guition ESP32-S3-4848S040
 * 480×480 RGB panel, 8MB OPI PSRAM available
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Colour depth — 16-bit RGB565 for the ST7701S panel */
#define LV_COLOR_DEPTH 16

/* Display resolution */
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 480

/* Memory — use PSRAM for LVGL heap */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
  #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
  #define LV_MEM_CUSTOM_ALLOC   ps_malloc
  #define LV_MEM_CUSTOM_FREE    free
  #define LV_MEM_CUSTOM_REALLOC ps_realloc
#else
  #define LV_MEM_SIZE (64 * 1024U)
#endif

/* Double-buffer DMA rendering */
#define LV_COLOR_16_SWAP 0

/* Tick — wired to millis() in main.cpp */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE  <Arduino.h>
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/* Font */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/* Theme */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1

/* Widgets we use */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHART      0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     0
#define LV_USE_SLIDER     0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    1
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      0
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0
#define LV_USE_MSGBOX     1
#define LV_USE_KEYBOARD   0

/* Logging */
#define LV_USE_LOG        1
#define LV_LOG_LEVEL      LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF     1

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

#endif /* LV_CONF_H */
