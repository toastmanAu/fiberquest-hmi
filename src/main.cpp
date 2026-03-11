/**
 * FiberQuest HMI — main.cpp
 *
 * Hardware: Guition ESP32-S3-4848S040
 *   - 480×480 ST7701S RGB parallel display
 *   - GT911 capacitive touch
 *   - 16MB flash, 8MB OPI PSRAM
 *
 * UI framework: LVGL v8
 *
 * Role: Physical arcade-style control surface for FiberQuest tournaments.
 *   - Browse & join tournaments (QR entry or WiFi-direct)
 *   - Live score display (scores pushed from Node.js agent via WebSocket)
 *   - Fiber channel status + balance
 *   - Winner celebration screen
 */

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <GT911.h>
#include <Wire.h>

#include "pins.h"
#include "ui/screens.h"
#include "net/ws_client.h"
#include "net/wifi_setup.h"

// ── Display setup ────────────────────────────────────────────────────
Arduino_ESP32RGBPanel *rgbBus = new Arduino_ESP32RGBPanel(
    PIN_RGB_DE, PIN_RGB_VSYNC, PIN_RGB_HSYNC, PIN_RGB_PCLK,
    PIN_RGB_R0, PIN_RGB_R1, PIN_RGB_R2, PIN_RGB_R3, PIN_RGB_R4,
    PIN_RGB_G0, PIN_RGB_G1, PIN_RGB_G2, PIN_RGB_G3, PIN_RGB_G4, PIN_RGB_G5,
    PIN_RGB_B0, PIN_RGB_B1, PIN_RGB_B2, PIN_RGB_B3, PIN_RGB_B4);

Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
    rgbBus, GFX_NOT_DEFINED, 0, true,
    DISPLAY_W, DISPLAY_H,
    st7701_type1_init_operations, sizeof(st7701_type1_init_operations), true,
    10, 8, 50, 10, 8, 20);

// ── LVGL draw buffer (in PSRAM) ───────────────────────────────────────
static lv_disp_draw_buf_t drawBuf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static const size_t BUF_LINES = 20;

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
    lv_disp_flush_ready(drv);
}

// ── GT911 touch ───────────────────────────────────────────────────────
GT911 touch;

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    GTPoint p;
    int n = touch.readTouchPoints(&p, 1);
    if (n > 0) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = p.x;
        data->point.y = p.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ── LVGL tick (ISR-safe) ──────────────────────────────────────────────
static void IRAM_ATTR lv_tick_isr(void *arg) {
    lv_tick_inc(1);
}

// ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("[FQH] FiberQuest HMI booting...");

    // Backlight on
    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    // Init display
    gfx->begin();
    gfx->fillScreen(BLACK);
    Serial.println("[FQH] Display OK");

    // Init PSRAM buffers
    buf1 = (lv_color_t *)ps_malloc(DISPLAY_W * BUF_LINES * sizeof(lv_color_t));
    buf2 = (lv_color_t *)ps_malloc(DISPLAY_W * BUF_LINES * sizeof(lv_color_t));
    if (!buf1 || !buf2) {
        Serial.println("[FQH] PSRAM alloc failed!");
        while(1) delay(1000);
    }

    // Init LVGL
    lv_init();
    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, DISPLAY_W * BUF_LINES);

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res   = DISPLAY_W;
    dispDrv.ver_res   = DISPLAY_H;
    dispDrv.flush_cb  = lvgl_flush_cb;
    dispDrv.draw_buf  = &drawBuf;
    lv_disp_drv_register(&dispDrv);
    Serial.println("[FQH] LVGL display driver registered");

    // Init GT911 touch
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
    touch.begin(PIN_TOUCH_RST, PIN_TOUCH_INT, GT911_ADDR);
    Serial.println("[FQH] Touch OK");

    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type     = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb  = lvgl_touch_cb;
    lv_indev_drv_register(&indevDrv);

    // LVGL tick timer (1ms via ESP32 hw timer)
    esp_timer_handle_t tickTimer;
    esp_timer_create_args_t timerArgs = {
        .callback = lv_tick_isr,
        .name     = "lv_tick"
    };
    esp_timer_create(&timerArgs, &tickTimer);
    esp_timer_start_periodic(tickTimer, 1000); // 1ms

    // Show splash → WiFi setup screen
    ui_init();
    wifi_setup_begin();

    Serial.println("[FQH] Boot complete");
}

void loop() {
    lv_task_handler();
    ws_client_loop();
    delay(2);
}
