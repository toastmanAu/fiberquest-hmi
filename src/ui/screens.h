/**
 * screens.h — FiberQuest HMI screen definitions
 *
 * Screen flow:
 *   SPLASH → WIFI_SETUP → HOME → TOURNAMENT_LIST → TOURNAMENT_DETAIL
 *         ↘ LIVE_GAME ← (pushed by WebSocket agent event)
 *         ↘ WINNER    ← (pushed by WebSocket payout event)
 *         ↘ CHANNEL   ← (Fiber channel status)
 */
#pragma once
#include <lvgl.h>
#include "../net/ws_client.h"

// ── Screen handles ─────────────────────────────────────────────────────
extern lv_obj_t *scr_splash;
extern lv_obj_t *scr_home;
extern lv_obj_t *scr_tournaments;
extern lv_obj_t *scr_live;
extern lv_obj_t *scr_winner;
extern lv_obj_t *scr_channel;

// ── Colour palette ─────────────────────────────────────────────────────
#define COL_BG        lv_color_hex(0x0d1018)
#define COL_SURFACE   lv_color_hex(0x131720)
#define COL_ACCENT    lv_color_hex(0x4f8ef7)
#define COL_GREEN     lv_color_hex(0x34d399)
#define COL_YELLOW    lv_color_hex(0xfbbf24)
#define COL_RED       lv_color_hex(0xf87171)
#define COL_TEXT      lv_color_hex(0xe2e8f0)
#define COL_MUTED     lv_color_hex(0x8899aa)

// ── Lifecycle ──────────────────────────────────────────────────────────
void ui_init();
void ui_show_home();
void ui_show_tournaments();
void ui_show_live(const char *game_name, const char *player1, const char *player2);
void ui_show_winner(const char *winner_name, uint64_t payout_ckb_shannons);
void ui_show_channel();

// ── Live game update (called from ws_client on score event) ───────────
void ui_live_update_score(uint8_t player_idx, int32_t score);
void ui_live_set_timer(uint32_t seconds_remaining);
void ui_live_set_status(const char *msg);
