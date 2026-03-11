/**
 * ws_client.cpp — FiberQuest agent WebSocket client
 *
 * Parses incoming JSON events from the Node.js agent and calls
 * ui_* functions to update the display.
 *
 * Uses WebSockets library (links-a2/arduinoWebSockets).
 * Add to platformio.ini: links-a2/WebSockets @ ^2.4.1
 */
#include "ws_client.h"
#include "../ui/screens.h"
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

static WebSocketsClient ws;
static bool _connected = false;
static char _host[64] = WS_DEFAULT_HOST;
static uint16_t _port = WS_DEFAULT_PORT;

static void ws_event_handler(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
    case WStype_CONNECTED:
        _connected = true;
        Serial.println("[WS] Connected to FiberQuest agent");
        ui_live_set_status("Agent connected");
        break;

    case WStype_DISCONNECTED:
        _connected = false;
        Serial.println("[WS] Disconnected");
        ui_live_set_status("Agent disconnected");
        break;

    case WStype_TEXT: {
        // Parse JSON event
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload, length);
        if (err) {
            Serial.printf("[WS] JSON parse error: %s\n", err.c_str());
            break;
        }

        const char *event = doc["event"] | "";

        if (strcmp(event, "score") == 0) {
            uint8_t player = doc["player"] | 0;
            int32_t value  = doc["value"]  | 0;
            ui_live_update_score(player, value);

        } else if (strcmp(event, "timer") == 0) {
            uint32_t secs = doc["seconds"] | 0;
            ui_live_set_timer(secs);

        } else if (strcmp(event, "status") == 0) {
            const char *msg = doc["msg"] | "";
            ui_live_set_status(msg);

        } else if (strcmp(event, "winner") == 0) {
            const char *name     = doc["name"]            | "Unknown";
            uint64_t shannons    = doc["payout_shannons"] | (uint64_t)0;
            ui_show_winner(name, shannons);

        } else if (strcmp(event, "game_start") == 0) {
            const char *game = doc["game"]    | "Unknown Game";
            const char *p1   = doc["player1"] | "Player 1";
            const char *p2   = doc["player2"] | "Player 2";
            ui_show_live(game, p1, p2);

        } else if (strcmp(event, "tournament_list") == 0) {
            // TODO: populate ui_show_tournaments() list from doc["items"]
            Serial.println("[WS] tournament_list received (TODO: populate UI)");
        }
        break;
    }

    case WStype_ERROR:
        Serial.println("[WS] Error");
        break;

    default:
        break;
    }
}

void ws_client_begin() {
    Serial.printf("[WS] Connecting to ws://%s:%u\n", _host, _port);
    ws.begin(_host, _port, "/");
    ws.onEvent(ws_event_handler);
    ws.setReconnectInterval(3000);
    ws.enableHeartbeat(15000, 3000, 2);
}

void ws_client_loop() {
    ws.loop();
}

bool ws_client_connected() { return _connected; }

void ws_client_set_host(const char *host, uint16_t port) {
    strncpy(_host, host, sizeof(_host) - 1);
    _port = port;
}
