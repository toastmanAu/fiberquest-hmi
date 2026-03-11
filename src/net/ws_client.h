/**
 * ws_client.h — WebSocket client for FiberQuest HMI
 *
 * Connects to the FiberQuest Node.js agent (Electron/server process)
 * and receives real-time game events:
 *
 *   { "event": "score", "player": 0, "value": 42 }
 *   { "event": "timer", "seconds": 87 }
 *   { "event": "tournament_list", "items": [...] }
 *   { "event": "winner", "name": "Alice", "payout_shannons": 50000000000 }
 *   { "event": "status", "msg": "Waiting for players..." }
 *
 * Agent URL configured via NVS or compiled-in default.
 * Default: ws://fiberquest.local:8765
 */
#pragma once
#include <stdint.h>

#define WS_DEFAULT_HOST "fiberquest.local"
#define WS_DEFAULT_PORT 8765

void ws_client_begin();
void ws_client_loop();
bool ws_client_connected();
void ws_client_set_host(const char *host, uint16_t port);
