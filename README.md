# FiberQuest HMI

> ESP32-S3 physical display for FiberQuest retro gaming tournaments.

Runs on a **Guition ESP32-S3-4848S040** — 4" 480×480 RGB panel, GT911 capacitive touch, 16MB flash, 8MB OPI PSRAM. Displays live tournament state and receives real-time score updates from the FiberQuest Node.js agent over WebSocket.

## Hardware

| Part | Detail |
|------|--------|
| Board | Guition ESP32-S3-4848S040 |
| Display | ST7701S, 480×480, 16-bit RGB parallel |
| Touch | GT911, I2C (SDA=19, SCL=45) |
| Flash | 16MB |
| PSRAM | 8MB OPI (used for LVGL draw buffers) |

## Screens

```
SPLASH → HOME → TOURNAMENT_LIST → [join] ─→ LIVE_GAME
                       ↑                        ↓ (winner event)
                    CHANNEL ←──── HOME ←── WINNER SCREEN
```

- **Splash** — boot logo + spinner
- **Home** — Join Tournament / Fiber Channel buttons
- **Tournaments** — live list from agent WebSocket
- **Live Game** — real-time score cards (2 players), timer bar
- **Winner** — celebration screen + Fiber payout amount
- **Channel** — Fiber channel status + balance

## Architecture

```
FiberQuest Node.js Agent (PC/Pi)
    WebSocket server → ws://fiberquest.local:8765
         ↓ JSON events
ESP32-S3 HMI
    ws_client.cpp → parses events → ui/screens.cpp → LVGL render
```

## Event protocol (from agent)

```json
{ "event": "game_start",    "game": "sf2-turbo", "player1": "Alice", "player2": "Bob" }
{ "event": "score",         "player": 0, "value": 42 }
{ "event": "timer",         "seconds": 87 }
{ "event": "status",        "msg": "Waiting for players..." }
{ "event": "winner",        "name": "Alice", "payout_shannons": 50000000000 }
{ "event": "tournament_list", "items": [ ... ] }
```

## Build & Flash

```bash
# Install PlatformIO (if not already)
pip install platformio

# Build
pio run -e fiberquest-hmi

# Flash (board connected via USB)
pio run -e fiberquest-hmi -t upload

# Serial monitor
pio device monitor
```

## WiFi Config

On first build: set SSID/password in NVS via Serial or add to `wifi_setup.cpp`. Full on-screen keyboard provisioning is a TODO (LVGL keyboard widget ready in lv_conf.h).

## TODO

- [ ] WiFi provisioning UI (on-screen keyboard → NVS)
- [ ] Tournament list populated from `tournament_list` WebSocket event
- [ ] Agent host/port configurable from settings screen
- [ ] QR code display for tournament entry (WyQR.h available from wyltek-embedded-builder)
- [ ] mDNS discovery of `fiberquest.local` agent
- [ ] Add WebSockets lib to platformio.ini (`links-a2/WebSockets @ ^2.4.1`)
- [ ] Score bar/chart instead of plain number labels
- [ ] Multi-player (2–4) layout

## Related

- [FiberQuest](https://github.com/toastmanAu/fiberquest) — Node.js agent
- [wyltek-embedded-builder](https://github.com/toastmanAu/wyltek-embedded-builder) — WyDisplay, WyQR, WyTouch headers
