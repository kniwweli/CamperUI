#include "ui_main.h"
#include <math.h>

static lv_obj_t *gps_map;
static lv_obj_t *gps_marker;
static lv_obj_t *lbl_gps_status;
static lv_obj_t *lbl_gps_coords;
static lv_obj_t *lbl_gps_details;

void ui_gps_reset() {
    gps_map = nullptr;
    gps_marker = nullptr;
    lbl_gps_status = nullptr;
    lbl_gps_coords = nullptr;
    lbl_gps_details = nullptr;
}

static lv_obj_t *gps_label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static void add_map_line(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, w, h);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_bg_color(line, ui_theme_border(), 0);
}

void ui_build_gps(lv_obj_t *parent) {
    ui_gps_reset();
    ui_setup_tab_page(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = ui_create_card(parent, 440, 390);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(card, 14, 0);

    lv_obj_t *title = gps_label(card, "GPS / Aktuelle Position", &lv_font_montserrat_18, ui_theme_text());
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 0);

    lbl_gps_status = gps_label(card, "Kein GPS-Fix", &lv_font_montserrat_14, lv_color_hex(UI_COLOR_WARNING));
    lv_obj_align(lbl_gps_status, LV_ALIGN_TOP_RIGHT, -4, 2);

    // Lightweight native map canvas: no tile downloads or browser required.
    gps_map = lv_obj_create(card);
    lv_obj_set_size(gps_map, 408, 215);
    lv_obj_align(gps_map, LV_ALIGN_TOP_MID, 0, 34);
    lv_obj_set_style_radius(gps_map, 10, 0);
    lv_obj_set_style_bg_color(gps_map, state.dark_mode ? lv_color_hex(0x182832) : lv_color_hex(0xe8f1ed), 0);
    lv_obj_set_style_border_width(gps_map, 1, 0);
    lv_obj_set_style_border_color(gps_map, ui_theme_border(), 0);
    lv_obj_clear_flag(gps_map, LV_OBJ_FLAG_SCROLLABLE);
    for (int x = 70; x < 408; x += 70) add_map_line(gps_map, x, 0, 1, 215);
    for (int y = 43; y < 215; y += 43) add_map_line(gps_map, 0, y, 408, 1);
    lv_obj_t *north = gps_label(gps_map, "N", &lv_font_montserrat_12, ui_theme_muted());
    lv_obj_align(north, LV_ALIGN_TOP_RIGHT, -8, 6);
    gps_marker = lv_obj_create(gps_map);
    lv_obj_set_size(gps_marker, 18, 18);
    lv_obj_set_style_radius(gps_marker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(gps_marker, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_set_style_border_width(gps_marker, 3, 0);
    lv_obj_set_style_border_color(gps_marker, lv_color_hex(0xffffff), 0);
    lv_obj_center(gps_marker);

    lbl_gps_coords = gps_label(card, "Position wartet auf GPS-Daten", &lv_font_montserrat_16, ui_theme_text());
    lv_obj_align(lbl_gps_coords, LV_ALIGN_TOP_LEFT, 8, 264);
    lbl_gps_details = gps_label(card, "Genauigkeit: --    Geschwindigkeit: --", &lv_font_montserrat_12, ui_theme_muted());
    lv_obj_align(lbl_gps_details, LV_ALIGN_TOP_LEFT, 8, 292);
}

void ui_update_gps_tab() {
    if (!lbl_gps_status || !lbl_gps_coords || !lbl_gps_details || !gps_marker) return;
    const bool valid = state.gps_fix_valid && state.gps_latitude != 0.0 && state.gps_longitude != 0.0;
    const uint32_t age = state.gps_updated_ms ? (millis() - state.gps_updated_ms) / 1000UL : 0;
    if (!valid) {
        ui_label_set_text_if_changed(lbl_gps_status, "Kein GPS-Fix");
        ui_text_color_if_changed(lbl_gps_status, lv_color_hex(UI_COLOR_WARNING));
        ui_label_set_text_if_changed(lbl_gps_coords, "Position wartet auf GPS-Daten");
        ui_label_set_text_if_changed(lbl_gps_details, "VanPi: keine Koordinaten im /position_sensor-Payload");
        return;
    }
    char coords[96];
    snprintf(coords, sizeof(coords), "%+.6f, %+.6f", state.gps_latitude, state.gps_longitude);
    ui_label_set_text_if_changed(lbl_gps_coords, coords);
    char details[128];
    snprintf(details, sizeof(details), "Genauigkeit: %.1f m   %.1f km/h   Kurs %.0f°   Fix vor %lus",
             state.gps_accuracy_m, state.gps_speed_kmh, state.gps_heading_deg, (unsigned long)age);
    ui_label_set_text_if_changed(lbl_gps_details, details);
    ui_label_set_text_if_changed(lbl_gps_status, age < 15 ? "GPS aktiv" : "GPS veraltet");
    ui_text_color_if_changed(lbl_gps_status, age < 15 ? lv_color_hex(UI_COLOR_SUCCESS) : lv_color_hex(UI_COLOR_WARNING));
}
