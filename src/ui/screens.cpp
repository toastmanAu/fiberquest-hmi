/**
 * screens.cpp — FiberQuest HMI LVGL screens
 *
 * All screens use the same dark theme matching wyltekindustries.com.
 * Font: LV_FONT_MONTSERRAT_* (built in via lv_conf.h)
 */
#include "screens.h"
#include <Arduino.h>

lv_obj_t *scr_splash      = nullptr;
lv_obj_t *scr_home        = nullptr;
lv_obj_t *scr_tournaments = nullptr;
lv_obj_t *scr_live        = nullptr;
lv_obj_t *scr_winner      = nullptr;
lv_obj_t *scr_channel     = nullptr;

// Live game widgets (updated by ws_client events)
static lv_obj_t *_live_score0 = nullptr;
static lv_obj_t *_live_score1 = nullptr;
static lv_obj_t *_live_timer  = nullptr;
static lv_obj_t *_live_status = nullptr;

// ── Helpers ───────────────────────────────────────────────────────────
static void set_bg(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

static lv_obj_t *label(lv_obj_t *parent, const char *txt,
                        const lv_font_t *font, lv_color_t col,
                        lv_align_t align, int32_t x, int32_t y) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, col, 0);
    lv_obj_align(l, align, x, y);
    return l;
}

static lv_obj_t *card(lv_obj_t *parent, int32_t w, int32_t h,
                       lv_align_t align, int32_t x, int32_t y) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, h);
    lv_obj_align(c, align, x, y);
    lv_obj_set_style_bg_color(c, COL_SURFACE, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(0x1e2535), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c, 10, 0);
    lv_obj_set_style_pad_all(c, 12, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

// ── Splash screen ─────────────────────────────────────────────────────
static void build_splash() {
    scr_splash = lv_obj_create(NULL);
    set_bg(scr_splash);

    label(scr_splash, "🎮 FIBERQUEST", &lv_font_montserrat_28, COL_ACCENT,
          LV_ALIGN_CENTER, 0, -40);
    label(scr_splash, "retro gaming · fiber payments", &lv_font_montserrat_16, COL_MUTED,
          LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *spinner = lv_spinner_create(scr_splash, 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_set_style_arc_color(spinner, COL_ACCENT, LV_PART_INDICATOR);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 80);
}

// ── Home screen ───────────────────────────────────────────────────────
static void _on_join_press(lv_event_t *e) {
    ui_show_tournaments();
}
static void _on_channel_press(lv_event_t *e) {
    ui_show_channel();
}

static void build_home() {
    scr_home = lv_obj_create(NULL);
    set_bg(scr_home);

    label(scr_home, "FIBERQUEST", &lv_font_montserrat_28, COL_ACCENT,
          LV_ALIGN_TOP_MID, 0, 24);
    label(scr_home, "Wyltek Industries", &lv_font_montserrat_12, COL_MUTED,
          LV_ALIGN_TOP_MID, 0, 62);

    // Join tournament button
    lv_obj_t *btn_join = lv_btn_create(scr_home);
    lv_obj_set_size(btn_join, 360, 72);
    lv_obj_align(btn_join, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(btn_join, COL_ACCENT, 0);
    lv_obj_set_style_radius(btn_join, 12, 0);
    lv_obj_add_event_cb(btn_join, _on_join_press, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_join_label = lv_label_create(btn_join);
    lv_label_set_text(btn_join_label, "🎮  Join Tournament");
    lv_obj_set_style_text_font(btn_join_label, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_join_label);

    // Channel status button
    lv_obj_t *btn_ch = lv_btn_create(scr_home);
    lv_obj_set_size(btn_ch, 360, 60);
    lv_obj_align(btn_ch, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(btn_ch, COL_SURFACE, 0);
    lv_obj_set_style_border_color(btn_ch, lv_color_hex(0x1e2535), 0);
    lv_obj_set_style_border_width(btn_ch, 1, 0);
    lv_obj_set_style_radius(btn_ch, 12, 0);
    lv_obj_add_event_cb(btn_ch, _on_channel_press, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_ch_label = lv_label_create(btn_ch);
    lv_label_set_text(btn_ch_label, "⚡  Fiber Channel");
    lv_obj_set_style_text_font(btn_ch_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(btn_ch_label, COL_TEXT, 0);
    lv_obj_center(btn_ch_label);

    // Status bar at bottom
    label(scr_home, "Connecting to agent...", &lv_font_montserrat_12, COL_MUTED,
          LV_ALIGN_BOTTOM_MID, 0, -16);
}

// ── Tournament list ───────────────────────────────────────────────────
// Populated dynamically from WebSocket tournament_list event
static lv_obj_t *_tournament_list = nullptr;

static void build_tournaments() {
    scr_tournaments = lv_obj_create(NULL);
    set_bg(scr_tournaments);

    label(scr_tournaments, "Tournaments", &lv_font_montserrat_24, COL_TEXT,
          LV_ALIGN_TOP_LEFT, 20, 20);
    label(scr_tournaments, "← Back", &lv_font_montserrat_14, COL_ACCENT,
          LV_ALIGN_TOP_RIGHT, -20, 24);

    _tournament_list = lv_list_create(scr_tournaments);
    lv_obj_set_size(_tournament_list, 440, 380);
    lv_obj_align(_tournament_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(_tournament_list, COL_BG, 0);
    lv_obj_set_style_border_width(_tournament_list, 0, 0);

    // Placeholder — real items added by ui_add_tournament()
    lv_list_add_text(_tournament_list, "Loading from agent...");
}

// ── Live game screen ──────────────────────────────────────────────────
static void build_live(const char *game_name, const char *p1, const char *p2) {
    scr_live = lv_obj_create(NULL);
    set_bg(scr_live);

    // Game title
    label(scr_live, game_name, &lv_font_montserrat_20, COL_ACCENT,
          LV_ALIGN_TOP_MID, 0, 16);

    // Player 1 card (left)
    lv_obj_t *card1 = card(scr_live, 200, 180, LV_ALIGN_LEFT_MID, 16, -30);
    label(card1, p1, &lv_font_montserrat_14, COL_TEXT, LV_ALIGN_TOP_MID, 0, 4);
    _live_score0 = label(card1, "0", &lv_font_montserrat_36, COL_GREEN,
                         LV_ALIGN_CENTER, 0, 10);

    // VS
    label(scr_live, "VS", &lv_font_montserrat_24, COL_MUTED,
          LV_ALIGN_CENTER, 0, -30);

    // Player 2 card (right)
    lv_obj_t *card2 = card(scr_live, 200, 180, LV_ALIGN_RIGHT_MID, -16, -30);
    label(card2, p2, &lv_font_montserrat_14, COL_TEXT, LV_ALIGN_TOP_MID, 0, 4);
    _live_score1 = label(card2, "0", &lv_font_montserrat_36, COL_YELLOW,
                         LV_ALIGN_CENTER, 0, 10);

    // Timer bar
    lv_obj_t *timer_bar = lv_bar_create(scr_live);
    lv_obj_set_size(timer_bar, 420, 12);
    lv_obj_align(timer_bar, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_bg_color(timer_bar, lv_color_hex(0x1e2535), 0);
    lv_obj_set_style_bg_color(timer_bar, COL_ACCENT, LV_PART_INDICATOR);
    lv_bar_set_range(timer_bar, 0, 100);
    lv_bar_set_value(timer_bar, 100, LV_ANIM_OFF);

    _live_timer  = label(scr_live, "3:00", &lv_font_montserrat_16, COL_TEXT,
                         LV_ALIGN_BOTTOM_MID, 0, -30);
    _live_status = label(scr_live, "LIVE", &lv_font_montserrat_12, COL_GREEN,
                         LV_ALIGN_BOTTOM_MID, 0, -14);
}

// ── Winner screen ─────────────────────────────────────────────────────
static void build_winner(const char *winner, uint64_t shannons) {
    scr_winner = lv_obj_create(NULL);
    set_bg(scr_winner);

    label(scr_winner, "🏆", &lv_font_montserrat_36, COL_YELLOW,
          LV_ALIGN_CENTER, 0, -80);
    label(scr_winner, winner, &lv_font_montserrat_28, COL_TEXT,
          LV_ALIGN_CENTER, 0, -20);
    label(scr_winner, "WINS!", &lv_font_montserrat_24, COL_GREEN,
          LV_ALIGN_CENTER, 0, 20);

    char payout[48];
    snprintf(payout, sizeof(payout), "Paid via Fiber: %.4f CKB",
             (double)shannons / 1e8);
    label(scr_winner, payout, &lv_font_montserrat_14, COL_MUTED,
          LV_ALIGN_CENTER, 0, 70);

    label(scr_winner, "Tap to return home", &lv_font_montserrat_12, COL_MUTED,
          LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(scr_winner, [](lv_event_t *e){ ui_show_home(); },
                        LV_EVENT_CLICKED, NULL);
}

// ── Channel screen ────────────────────────────────────────────────────
static void build_channel() {
    scr_channel = lv_obj_create(NULL);
    set_bg(scr_channel);

    label(scr_channel, "⚡ Fiber Channel", &lv_font_montserrat_20, COL_ACCENT,
          LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *c = card(scr_channel, 420, 280, LV_ALIGN_CENTER, 0, 10);
    label(c, "Agent Node ID", &lv_font_montserrat_12, COL_MUTED, LV_ALIGN_TOP_LEFT, 0, 0);
    label(c, "026a9dd1bae2...", &lv_font_montserrat_12, COL_TEXT, LV_ALIGN_TOP_LEFT, 0, 18);

    label(c, "Balance", &lv_font_montserrat_12, COL_MUTED, LV_ALIGN_TOP_LEFT, 0, 50);
    label(c, "— CKB", &lv_font_montserrat_20, COL_GREEN, LV_ALIGN_TOP_LEFT, 0, 68);

    label(c, "Status", &lv_font_montserrat_12, COL_MUTED, LV_ALIGN_TOP_LEFT, 0, 110);
    label(c, "Fetching...", &lv_font_montserrat_14, COL_TEXT, LV_ALIGN_TOP_LEFT, 0, 128);

    label(scr_channel, "← Back", &lv_font_montserrat_14, COL_ACCENT,
          LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(scr_channel, [](lv_event_t *e){ ui_show_home(); },
                        LV_EVENT_CLICKED, NULL);
}

// ── Public API ────────────────────────────────────────────────────────
void ui_init() {
    build_splash();
    lv_scr_load(scr_splash);
}

void ui_show_home() {
    if (!scr_home) build_home();
    lv_scr_load_anim(scr_home, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}

void ui_show_tournaments() {
    if (!scr_tournaments) build_tournaments();
    lv_scr_load_anim(scr_tournaments, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

void ui_show_live(const char *game_name, const char *player1, const char *player2) {
    if (scr_live) { lv_obj_del(scr_live); scr_live = nullptr; }
    build_live(game_name, player1, player2);
    lv_scr_load_anim(scr_live, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}

void ui_show_winner(const char *winner_name, uint64_t shannons) {
    if (scr_winner) { lv_obj_del(scr_winner); scr_winner = nullptr; }
    build_winner(winner_name, shannons);
    lv_scr_load_anim(scr_winner, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
}

void ui_show_channel() {
    if (!scr_channel) build_channel();
    lv_scr_load_anim(scr_channel, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

void ui_live_update_score(uint8_t player_idx, int32_t score) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", score);
    if (player_idx == 0 && _live_score0) lv_label_set_text(_live_score0, buf);
    if (player_idx == 1 && _live_score1) lv_label_set_text(_live_score1, buf);
}

void ui_live_set_timer(uint32_t secs) {
    if (!_live_timer) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%u:%02u", secs / 60, secs % 60);
    lv_label_set_text(_live_timer, buf);
}

void ui_live_set_status(const char *msg) {
    if (_live_status) lv_label_set_text(_live_status, msg);
}
