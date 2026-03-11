/**
 * wifi_setup.cpp — WiFi provisioning + auto-connect
 *
 * TODO: Replace hardcoded fallback with NVS-stored credentials + on-screen
 * keyboard entry (LVGL keyboard widget) for first-boot provisioning.
 */
#include "wifi_setup.h"
#include "../ui/screens.h"
#include "ws_client.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Arduino.h>

static Preferences prefs;
static bool _connected = false;
static char _ip[20] = "";

// ── Hardcoded dev fallback — replace with NVS provisioning ────────────
// These are read from NVS on boot; if absent, show setup UI.
#define FALLBACK_SSID     ""
#define FALLBACK_PASS     ""

static TaskHandle_t _wifi_task_handle = nullptr;

static void wifi_task(void *param) {
    prefs.begin("fq-wifi", false);
    String ssid = prefs.getString("ssid", FALLBACK_SSID);
    String pass = prefs.getString("pass", FALLBACK_PASS);
    prefs.end();

    if (ssid.isEmpty()) {
        Serial.println("[WiFi] No credentials — show setup UI");
        // TODO: ui_show_wifi_setup();
        vTaskDelete(NULL);
        return;
    }

    Serial.printf("[WiFi] Connecting to %s ...\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) {
        delay(500);
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        _connected = true;
        WiFi.localIP().toString().toCharArray(_ip, sizeof(_ip));
        Serial.printf("[WiFi] Connected — IP: %s\n", _ip);
        // Transition to home and start WebSocket
        ui_show_home();
        ws_client_begin();
    } else {
        Serial.println("[WiFi] Connection failed");
        // TODO: ui_show_wifi_error();
    }
    vTaskDelete(NULL);
}

void wifi_setup_begin() {
    xTaskCreatePinnedToCore(wifi_task, "wifi_setup", 4096, NULL, 1,
                            &_wifi_task_handle, 0);
}

bool wifi_is_connected() { return _connected; }
const char* wifi_get_ip() { return _ip; }
