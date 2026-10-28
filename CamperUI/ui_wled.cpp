#include "ui_wled.h"
#include "ui_main.h"
#include "wled_client.h"
#include "system_state.h"

static lv_obj_t *btn_power    = nullptr;
static lv_obj_t *lbl_power    = nullptr;
static lv_obj_t *slider_bri   = nullptr;
static lv_obj_t *lbl_bri      = nullptr;
static lv_obj_t *slider_white = nullptr;
static lv_obj_t *lbl_white    = nullptr;
static lv_obj_t *lbl_no_ip    = nullptr;

// 8 preset colors: name, R, G, B
static const struct { const char *name; uint8_t r, g, b; } PRESETS[] = {
    {"Warm",   255, 180,  80},
    {"Weiß",   255, 255, 255},
    {"Rot",    255,  30,  30},
    {"Grün",    30, 220,  80},
    {"Blau",    30, 100, 255},
    {"Cyan",    30, 220, 220},
    {"Lila",   160,  30, 220},
    {"Orange", 255, 110,  20},
};
static constexpr int PRESET_COUNT = 8;
static lv_obj_t *btn_preset[PRESET_COUNT] = {};

void ui_wled_reset(void) {
    btn_power    = nullptr;
    lbl_power    = nullptr;
    slider_bri   = nullptr;
    lbl_bri      = nullptr;
    slider_white = nullptr;
    lbl_white    = nullptr;
    lbl_no_ip    = nullptr;
    for (int i = 0; i < PRESET_COUNT; ++i) btn_preset[i] = nullptr;
}

// ─── event callbacks ──────────────────────────────────────────────────────────

static void cb_power(lv_event_t *) {
    state.wled_on = !state.wled_on;
    wled_set_power(state.wled_on);
    ui_update_wled();
}

static void cb_preset(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    state.wled_r = PRESETS[idx].r;
    state.wled_g = PRESETS[idx].g;
    state.wled_b = PRESETS[idx].b;
    state.wled_w = 0;
    state.wled_on = true;
    wled_set_state(true, state.wled_r, state.wled_g, state.wled_b, 0, state.wled_bri);
    ui_update_wled();
}

static void cb_slider_bri(lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    state.wled_bri = (uint8_t)lv_slider_get_value(sl);
    wled_set_brightness(state.wled_bri);
    ui_update_wled();
}

static void cb_slider_white(lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_target(e);
    state.wled_w = (uint8_t)lv_slider_get_value(sl);
    wled_set_white(state.wled_w);
    ui_update_wled();
}

// ─── builder ─────────────────────────────────────────────────────────────────

void ui_build_wled(lv_obj_t *parent) {
    ui_wled_reset();
    ui_setup_tab_page(parent);

    // ── Card 1: Power ───────────────────────────────────────────────────────
    lv_obj_t *card_pwr = ui_create_card(parent, 440, 76);
    lv_obj_set_pos(card_pwr, 0, 0);
    lv_obj_clear_flag(card_pwr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(card_pwr, 10, 0);

    lv_obj_t *lbl_title = lv_label_create(card_pwr);
    lv_label_set_text(lbl_title, "WLED LICHT");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_title, ui_theme_muted(), 0);
    lv_obj_set_pos(lbl_title, 4, 2);

    btn_power = lv_btn_create(card_pwr);
    lv_obj_set_size(btn_power, 420, 46);
    lv_obj_set_pos(btn_power, 0, 22);
    lv_obj_set_style_radius(btn_power, ui_radius_button(), 0);
    lv_obj_set_style_border_width(btn_power, ui_border_width(), 0);
    lv_obj_set_style_shadow_width(btn_power, 0, 0);
    lv_obj_add_event_cb(btn_power, cb_power, LV_EVENT_CLICKED, nullptr);

    lbl_power = lv_label_create(btn_power);
    lv_obj_set_style_text_font(lbl_power, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_power);

    // ── Card 2: Color Presets ────────────────────────────────────────────────
    lv_obj_t *card_col = ui_create_card(parent, 440, 174);
    lv_obj_set_pos(card_col, 0, 88);
    lv_obj_clear_flag(card_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(card_col, 10, 0);

    lv_obj_t *lbl_col_title = lv_label_create(card_col);
    lv_label_set_text(lbl_col_title, "FARBE");
    lv_obj_set_style_text_font(lbl_col_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_col_title, ui_theme_muted(), 0);
    lv_obj_set_pos(lbl_col_title, 4, 2);

    // 4 buttons per row, 2 rows (8 total), 104px wide with 8px gaps
    const int BW = 100, BH = 56, GAPX = 8, GAPY = 8, OX = 0, OY = 20;
    for (int i = 0; i < PRESET_COUNT; ++i) {
        int col = i % 4;
        int row = i / 4;
        btn_preset[i] = lv_btn_create(card_col);
        lv_obj_set_size(btn_preset[i], BW, BH);
        lv_obj_set_pos(btn_preset[i], OX + col * (BW + GAPX), OY + row * (BH + GAPY));
        lv_obj_set_style_radius(btn_preset[i], ui_radius_button(), 0);
        lv_obj_set_style_border_width(btn_preset[i], ui_border_width(), 0);
        lv_obj_set_style_shadow_width(btn_preset[i], 0, 0);
        lv_color_t swatch = lv_color_make(PRESETS[i].r, PRESETS[i].g, PRESETS[i].b);
        lv_obj_set_style_bg_color(btn_preset[i], swatch, 0);
        lv_obj_set_style_border_color(btn_preset[i], ui_theme_border(), 0);
        lv_obj_add_event_cb(btn_preset[i], cb_preset, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn_preset[i]);
        lv_label_set_text(lbl, PRESETS[i].name);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
        lv_obj_center(lbl);
    }

    // ── Card 3: Brightness + White ──────────────────────────────────────────
    lv_obj_t *card_bri = ui_create_card(parent, 440, 150);
    lv_obj_set_pos(card_bri, 0, 274);
    lv_obj_clear_flag(card_bri, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(card_bri, 10, 0);

    lv_obj_t *lbl_bri_title = lv_label_create(card_bri);
    lv_label_set_text(lbl_bri_title, "HELLIGKEIT");
    lv_obj_set_style_text_font(lbl_bri_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_bri_title, ui_theme_muted(), 0);
    lv_obj_set_pos(lbl_bri_title, 4, 2);

    slider_bri = lv_slider_create(card_bri);
    lv_obj_set_size(slider_bri, 380, 28);
    lv_obj_set_pos(slider_bri, 0, 20);
    lv_slider_set_range(slider_bri, 1, 255);
    lv_slider_set_value(slider_bri, state.wled_bri, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_bri, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_bri, ui_theme_dimmer(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_bri, ui_theme_text(), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_bri, cb_slider_bri, LV_EVENT_VALUE_CHANGED, nullptr);

    lbl_bri = lv_label_create(card_bri);
    lv_obj_set_style_text_font(lbl_bri, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_bri, ui_theme_muted(), 0);
    lv_obj_set_pos(lbl_bri, 386, 24);

    lv_obj_t *lbl_white_title = lv_label_create(card_bri);
    lv_label_set_text(lbl_white_title, "WEISSKANAL");
    lv_obj_set_style_text_font(lbl_white_title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_white_title, ui_theme_muted(), 0);
    lv_obj_set_pos(lbl_white_title, 4, 60);

    slider_white = lv_slider_create(card_bri);
    lv_obj_set_size(slider_white, 380, 28);
    lv_obj_set_pos(slider_white, 0, 78);
    lv_slider_set_range(slider_white, 0, 255);
    lv_slider_set_value(slider_white, state.wled_w, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_white, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_white, lv_color_make(255, 255, 200), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_white, ui_theme_text(), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_white, cb_slider_white, LV_EVENT_VALUE_CHANGED, nullptr);

    lbl_white = lv_label_create(card_bri);
    lv_obj_set_style_text_font(lbl_white, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_white, ui_theme_muted(), 0);
    lv_obj_set_pos(lbl_white, 386, 82);

    // No-IP warning (hidden by default)
    lbl_no_ip = lv_label_create(parent);
    lv_label_set_text(lbl_no_ip, LV_SYMBOL_WARNING " Keine WLED IP konfiguriert\n(Einstellungen > WLED IP)");
    lv_obj_set_style_text_font(lbl_no_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_no_ip, ui_theme_muted(), 0);
    lv_obj_set_style_text_align(lbl_no_ip, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(lbl_no_ip, 60, 150);
    lv_obj_add_flag(lbl_no_ip, LV_OBJ_FLAG_HIDDEN);

    ui_update_wled();
}

// ─── updater ─────────────────────────────────────────────────────────────────

void ui_update_wled(void) {
    if (!btn_power) return;

    bool no_ip = state.wled_ip.length() == 0;

    // Show/hide no-IP warning
    if (lbl_no_ip) {
        if (no_ip) lv_obj_clear_flag(lbl_no_ip, LV_OBJ_FLAG_HIDDEN);
        else       lv_obj_add_flag(lbl_no_ip, LV_OBJ_FLAG_HIDDEN);
    }

    // Power button
    if (state.wled_on) {
        ui_bg_color_if_changed(btn_power, ui_theme_relay());
        ui_border_color_if_changed(btn_power, ui_theme_relay());
        ui_label_set_text_if_changed(lbl_power, LV_SYMBOL_POWER "  EIN");
        ui_text_color_if_changed(lbl_power, ui_theme_btn_active_text());
    } else {
        ui_bg_color_if_changed(btn_power, ui_theme_card());
        ui_border_color_if_changed(btn_power, ui_theme_border());
        ui_label_set_text_if_changed(lbl_power, LV_SYMBOL_POWER "  AUS");
        ui_text_color_if_changed(lbl_power, ui_theme_btn_text());
    }

    // Highlight active color preset
    for (int i = 0; i < PRESET_COUNT; ++i) {
        if (!btn_preset[i]) continue;
        bool active = state.wled_on &&
                      state.wled_r == PRESETS[i].r &&
                      state.wled_g == PRESETS[i].g &&
                      state.wled_b == PRESETS[i].b;
        lv_color_t swatch = lv_color_make(PRESETS[i].r, PRESETS[i].g, PRESETS[i].b);
        lv_obj_set_style_bg_color(btn_preset[i], swatch, 0);
        if (active) {
            lv_obj_set_style_border_color(btn_preset[i], ui_theme_primary(), 0);
            lv_obj_set_style_border_width(btn_preset[i], 3, 0);
        } else {
            lv_obj_set_style_border_color(btn_preset[i], ui_theme_border(), 0);
            lv_obj_set_style_border_width(btn_preset[i], ui_border_width(), 0);
        }
    }

    // Brightness
    if (slider_bri) {
        lv_slider_set_value(slider_bri, state.wled_bri, LV_ANIM_OFF);
    }
    if (lbl_bri) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", (int)(state.wled_bri * 100 / 255));
        ui_label_set_text_if_changed(lbl_bri, buf);
    }

    // White channel
    if (slider_white) {
        lv_slider_set_value(slider_white, state.wled_w, LV_ANIM_OFF);
    }
    if (lbl_white) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", (int)(state.wled_w * 100 / 255));
        ui_label_set_text_if_changed(lbl_white, buf);
    }
}
