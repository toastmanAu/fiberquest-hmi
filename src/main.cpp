/**
 * FiberQuest HMI — main.cpp
 * Guition ESP32-S3-4848S040 (WEB target)
 *
 * Uses wyltek-embedded-builder (WEB) for:
 *   - WyDisplay  — ST7701S 480×480 RGB init
 *   - WyTouch    — GT911 I2C capacitive touch
 *   - WySettings — NVS + captive portal WiFi provisioning
 *   - WyNet      — WiFi connect / reconnect / mDNS
 *
 * After WiFi: connects to FiberQuest Pi agent via WebSocket
 * and renders live game state on LVGL screens.
 */

#include <Arduino.h>

// ── WEB board support ─────────────────────────────────────────────
#include "wyltek-embedded-builder/src/boards.h"
#include "wyltek-embedded-builder/src/display/WyDisplay.h"
#include "wyltek-embedded-builder/src/touch/WyTouch.h"
#include "wyltek-embedded-builder/src/settings/WySettings.h"
#include "wyltek-embedded-builder/src/net/WyNet.h"

// ── LVGL ──────────────────────────────────────────────────────────
#include <lvgl.h>
#include "lv_conf.h"

// ── App ───────────────────────────────────────────────────────────
#include "net/ws_client.h"
#include "ui/screens.h"

// ── Globals ───────────────────────────────────────────────────────
static WyDisplay   wyDisplay;
static WyTouch     wyTouch;
static WySettings  wySettings;
static WyNet       wyNet;

// LVGL draw buffer — in PSRAM
static lv_color_t  *lv_buf1 = nullptr;
static lv_color_t  *lv_buf2 = nullptr;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;

// ── LVGL display flush ────────────────────────────────────────────
static void lv_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    auto *gfx = (Arduino_GFX *)drv->user_data;
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
    lv_disp_flush_ready(drv);
}

// ── LVGL touch read ───────────────────────────────────────────────
static void lv_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    auto *touch = (WyTouch *)drv->user_data;
    uint16_t tx, ty;
    if (touch->read(tx, ty)) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = tx;
        data->point.y = ty;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ── LVGL tick (hardware timer) ────────────────────────────────────
static hw_timer_t *lvgl_timer = nullptr;
static void IRAM_ATTR lv_tick_isr() { lv_tick_inc(5); }

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[FiberQuest HMI] boot");

    // ── Display ────────────────────────────────────────────────────
    wyDisplay.begin();
    wyDisplay.setBrightness(200);

    // ── Touch ──────────────────────────────────────────────────────
    wyTouch.begin();

    // ── LVGL ───────────────────────────────────────────────────────
    lv_init();

    // Allocate draw buffers from PSRAM
    size_t buf_sz = WY_SCREEN_W * 40;  // 40 lines
    lv_buf1 = (lv_color_t *)heap_caps_malloc(buf_sz * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_buf2 = (lv_color_t *)heap_caps_malloc(buf_sz * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!lv_buf1 || !lv_buf2) {
        Serial.println("[LVGL] PSRAM alloc failed — falling back to internal RAM");
        lv_buf1 = (lv_color_t *)malloc(buf_sz * sizeof(lv_color_t));
        lv_buf2 = (lv_color_t *)malloc(buf_sz / 2 * sizeof(lv_color_t));
    }
    lv_disp_draw_buf_init(&draw_buf, lv_buf1, lv_buf2, buf_sz);

    // Register display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res    = WY_SCREEN_W;
    disp_drv.ver_res    = WY_SCREEN_H;
    disp_drv.flush_cb   = lv_flush_cb;
    disp_drv.draw_buf   = &draw_buf;
    disp_drv.user_data  = wyDisplay.gfx;
    lv_disp_drv_register(&disp_drv);

    // Register touch input driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type      = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb   = lv_touch_cb;
    indev_drv.user_data = &wyTouch;
    lv_indev_drv_register(&indev_drv);

    // Hardware timer for LVGL tick (5ms)
    lvgl_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(lvgl_timer, lv_tick_isr, true);
    timerAlarmWrite(lvgl_timer, 5000, true);
    timerAlarmEnable(lvgl_timer);

    // ── UI: show splash while connecting ───────────────────────────
    ui_init();
    lv_task_handler();

    // ── Settings / WiFi provisioning ──────────────────────────────
    wySettings.addString("ssid",       "WiFi SSID",       "");
    wySettings.addString("pass",       "WiFi Password",   "");
    wySettings.addString("agent_host", "Agent Host/IP",   "fiberquest.local");
    wySettings.addInt   ("agent_port", "Agent WS Port",   8765);

    wySettings.begin("fiberquest");

    if (wySettings.portalActive()) {
        // First boot or BOOT button held — show captive portal
        Serial.println("[Settings] Portal active — connect to WY-Setup-XXXX AP");
        while (wySettings.portalActive()) {
            wySettings.portalLoop();
            lv_task_handler();
        }
        Serial.println("[Settings] Saved — restarting");
        delay(500);
        ESP.restart();
    }

    const char *ssid   = wySettings.getString("ssid");
    const char *pass   = wySettings.getString("pass");
    const char *ahost  = wySettings.getString("agent_host");
    int         aport  = wySettings.getInt("agent_port");

    if (!ssid || strlen(ssid) == 0) {
        Serial.println("[WiFi] No SSID — entering portal");
        wySettings.startPortal();
        while (wySettings.portalActive()) {
            wySettings.portalLoop();
            lv_task_handler();
        }
        ESP.restart();
    }

    // ── Connect to WiFi ────────────────────────────────────────────
    wyNet.setHostname("fiberquest-hmi");
    wyNet.onConnect([ahost, aport]() {
        Serial.printf("[WiFi] Connected. Connecting to agent ws://%s:%d\n", ahost, aport);
        ws_client_set_host(ahost, (uint16_t)aport);
        ws_client_begin();
        ui_show_home();
    });
    wyNet.onDisconnect([]() {
        Serial.println("[WiFi] Disconnected");
        ui_live_set_status("WiFi lost...");
    });
    wyNet.begin(ssid, pass);

    Serial.println("[Setup] done");
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
    wyNet.loop();          // reconnect watchdog + OTA
    ws_client_loop();      // WebSocket event pump
    lv_task_handler();     // LVGL render
    delay(5);
}
