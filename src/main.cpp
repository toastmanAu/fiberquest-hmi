/**
 * FiberQuest HMI — main.cpp
 * Guition ESP32-S3-4848S040 (WEB target)
 */

#include <Arduino.h>
#include "boards.h"
#include "display/WyDisplay.h"
#include "touch/WyTouch.h"
#include "settings/WySettings.h"
#include "net/WyNet.h"
#include <lvgl.h>
#include "lv_conf.h"
#include "net/ws_client.h"
#include "ui/screens.h"

// ── Globals ────────────────────────────────────────────────────────
static WyDisplay   wyDisplay;
static WyTouch     wyTouch;
static WySettings  wySettings;
static WyNet       wyNet;

static lv_color_t       *lv_buf1 = nullptr;
static lv_color_t       *lv_buf2 = nullptr;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;

// Agent connection (set from settings before WiFi connect)
static char _agent_host[64] = "fiberquest.local";
static uint16_t _agent_port = 8765;

// ── LVGL display flush ────────────────────────────────────────────
static void lv_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    Arduino_GFX *gfx = (Arduino_GFX *)drv->user_data;
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
    lv_disp_flush_ready(drv);
}

// ── LVGL touch read ───────────────────────────────────────────────
static void lv_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    wyTouch.update();
    if (wyTouch.pressed) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = wyTouch.x;
        data->point.y = wyTouch.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ── LVGL tick ─────────────────────────────────────────────────────


// ── WiFi callbacks (plain function pointers — no captures) ────────
static void on_wifi_connect() {
    Serial.printf("[WiFi] Connected — agent ws://%s:%u\n", _agent_host, _agent_port);
    ws_client_set_host(_agent_host, _agent_port);
    ws_client_begin();
    ui_show_home();
}

static void on_wifi_disconnect() {
    Serial.println("[WiFi] Disconnected");
    ui_live_set_status("WiFi lost...");
}

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[FiberQuest HMI] boot");

    // Display
    wyDisplay.begin();
    wyDisplay.setBrightness(200);

    // Touch
    wyTouch.begin();

    // LVGL
    lv_init();

    size_t buf_sz = WY_SCREEN_W * 40;
    lv_buf1 = (lv_color_t *)heap_caps_malloc(buf_sz * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_buf2 = (lv_color_t *)heap_caps_malloc(buf_sz * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!lv_buf1 || !lv_buf2) {
        Serial.println("[LVGL] PSRAM alloc failed — using internal RAM");
        lv_buf1 = (lv_color_t *)malloc(buf_sz * sizeof(lv_color_t));
        lv_buf2 = (lv_color_t *)malloc(buf_sz / 2 * sizeof(lv_color_t));
    }
    lv_disp_draw_buf_init(&draw_buf, lv_buf1, lv_buf2, buf_sz);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res   = WY_SCREEN_W;
    disp_drv.ver_res   = WY_SCREEN_H;
    disp_drv.flush_cb  = lv_flush_cb;
    disp_drv.draw_buf  = &draw_buf;
    disp_drv.user_data = wyDisplay.gfx;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_touch_cb;
    lv_indev_drv_register(&indev_drv);

    

    // Splash while connecting
    ui_init();
    lv_task_handler();

    // Settings / captive portal provisioning
    wySettings.addString("ssid",       "WiFi SSID",     "");
    wySettings.addString("pass",       "WiFi Password", "");
    wySettings.addString("agent_host", "Agent Host/IP", "fiberquest.local");
    wySettings.addInt   ("agent_port", "Agent Port",    8765);

    wySettings.begin("fiberquest");

    if (wySettings.portalActive()) {
        Serial.println("[Settings] Portal active — connect to WY-Setup-XXXX AP");
        while (wySettings.portalActive()) {
            wySettings.portalLoop();
            lv_task_handler();
        }
        delay(500);
        ESP.restart();
    }

    const char *ssid   = wySettings.getString("ssid");
    const char *pass   = wySettings.getString("pass");
    const char *ahost  = wySettings.getString("agent_host");
    int         aport  = wySettings.getInt   ("agent_port");

    strncpy(_agent_host, ahost, sizeof(_agent_host) - 1);
    _agent_port = (uint16_t)aport;

    // Connect WiFi
    wyNet.setHostname("fiberquest-hmi");
    wyNet.onConnect(on_wifi_connect);
    wyNet.onDisconnect(on_wifi_disconnect);
    wyNet.begin(ssid, pass);
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
    wyNet.loop();
    ws_client_loop();
    lv_task_handler();
    delay(5);
}
