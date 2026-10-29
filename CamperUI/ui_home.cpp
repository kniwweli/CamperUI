#include "ui_main.h"
#include "http_handler.h"
#include "buzzer.h"
#include "victron_client.h"
#include "ui_victron.h"
#include "ui_control_style.h"
#include "home_mp_activity.h"
#include <math.h>
namespace {
lv_obj_t *root;
lv_obj_t *favorites[2], *favorite_text[2], *temperatures[3], *temp_name[3], *temp_value[3];
lv_obj_t *battery, *bat_title, *bat_power, *bat_detail, *starter;
lv_obj_t *solar, *solar_power, *solar_detail, *water, *water_title, *water_value, *water_bar;
lv_obj_t *multiplus,*mp_power;
lv_obj_t *emer_btn = nullptr, *emer_bar = nullptr;
lv_timer_t *emer_timer = nullptr;
uint32_t emer_press_start = 0;
bool emer_triggered = false;
HomeMpActivity mp_activity;
uint8_t remembered_dimmer[8] = {};
lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, const lv_font_t *font) {
    lv_obj_t *obj = lv_label_create(parent); lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0); lv_obj_set_style_text_color(obj, ui_theme_text(), 0);
    lv_obj_set_pos(obj, x, y); return obj;
}
void visible(lv_obj_t *obj, bool show) {
    if (show) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
bool update_mp_activity() {
    auto gx=victron_snapshot();bool before=mp_activity.visible;
    mp_activity.update(state.home_show_victron&&state.victron_enabled,state.debug_mode,
        gx.demo==state.debug_mode&&victron_fresh(gx,GX_MODE),gx.values[GX_MODE].valid?(int)gx.values[GX_MODE].value:0);
    return before!=mp_activity.visible;
}
void emer_timer_cb(lv_timer_t *) {
    if (!emer_btn || !lv_obj_is_valid(emer_btn)) return;
    bool pressed = lv_obj_has_state(emer_btn, LV_STATE_PRESSED);
    if (pressed) {
        if (emer_press_start == 0) emer_press_start = millis();
        uint32_t elapsed = millis() - emer_press_start;
        if (emer_bar) lv_bar_set_value(emer_bar, (int)constrain((long)elapsed * 100 / 5000, 0L, 100L), LV_ANIM_OFF);
        if (!emer_triggered && elapsed >= 5000) {
            emer_triggered = true;
            http_publish_switch(7, true);
            buzzer_beep_force(500);
        }
    } else {
        if (emer_press_start != 0) {
            emer_press_start = 0;
            emer_triggered = false;
            if (emer_bar) lv_bar_set_value(emer_bar, 0, LV_ANIM_OFF);
        }
    }
}
void layout() {
    if (!root) return;
    bool compact=state.home_show_victron;
    int y = 0, count = (state.home_favorite[0] != 0) + (state.home_favorite[1] != 0), column = 0;
    for (int i = 0; i < 2; ++i) {
        bool show = state.home_favorite[i] != 0; visible(favorites[i], show); if (!show) continue;
        int width = count == 1 ? 464 : 228;
        lv_obj_set_pos(favorites[i], column++ * 236, y); lv_obj_set_width(favorites[i], width);
        lv_obj_set_width(favorite_text[i], width - 24); lv_obj_center(favorite_text[i]);
    }
    if (count) y += compact ? 60 : 64;
    count = column = 0;
    for (int i = 0; i < 3; ++i) if (state.home_show_temp[i] && ui_home_temperature_source(i) >= 0) ++count;
    for (int i = 0; i < 3; ++i) {
        bool show = state.home_show_temp[i] && ui_home_temperature_source(i) >= 0; visible(temperatures[i], show); if (!show) continue;
        int width = (464 - (count - 1) * 8) / count;
        lv_obj_set_pos(temperatures[i], column++ * (width + 8), y);
        lv_obj_set_width(temperatures[i], width); lv_obj_set_width(temp_name[i], width - 24);
    }
    if (count) y += compact ? 74 : 78;
    visible(battery, state.home_show_battery); visible(solar, state.home_show_solar);
    bool show_mp=mp_activity.visible && state.home_show_victron && state.victron_enabled;
    if(show_mp && !mp_power)mp_power=ui_build_victron_home_power(root);
    int power_count=(int)state.home_show_battery+(int)state.home_show_solar+(int)show_mp;
    bool three=power_count==3;
    int x=0;
    for(auto card:{battery,solar}) {
        bool show=card==battery?state.home_show_battery:state.home_show_solar;
        int width=three?172:power_count>1?228:464;
        lv_obj_set_width(card,width);lv_obj_set_style_pad_all(card,three?10:12,0);
        if(show){lv_obj_set_pos(card,x,y);x+=width+8;}
    }
    if(mp_power){visible(mp_power,show_mp);lv_obj_set_pos(mp_power,x,y);lv_obj_set_width(mp_power,three?104:power_count>1?228:464);}
    visible(starter, state.home_show_starter);
    if (power_count) y += compact ? 126 : 130;
    visible(water, state.home_show_water && !state.home_show_victron); lv_obj_set_pos(water, 0, y);
    if (state.home_show_victron && !multiplus) multiplus=ui_build_victron_home(root);
    if (multiplus) { visible(multiplus,state.home_show_victron);lv_obj_set_pos(multiplus,0,y); }
    if (emer_btn) visible(emer_btn, state.emergency_btn_enabled);
    lv_obj_update_layout(root);
    lv_obj_scroll_to_y(root, lv_obj_get_scroll_y(root), LV_ANIM_OFF);
}
bool source_ready() { return state.data_is_demo == state.debug_mode; }
bool favorite_ready(int selection) {
    if (selection==17 || selection==18) return victron_relay_available(victron_snapshot(),selection-17);
    if (!source_ready() || selection < 1 || selection > 16) return false;
    if (!state.debug_mode && (!state.wifi_connected || !state.vanpi_connected)) return false;
    return selection <= 8 ? (state.relay_fields & (1 << (selection - 1))) != 0 :
                           (state.dimmer_fields & (1 << (selection - 9))) != 0;
}
void favorite_click(lv_event_t *event) {
    int slot = (int)(uintptr_t)lv_event_get_user_data(event), selection = state.home_favorite[slot];
    if (!favorite_ready(selection)) return;
    if (selection >= 17) {
        auto gx = victron_snapshot();
        victron_request_relay(selection - 17, gx.values[GX_RELAY0 + selection - 17].value == 0);
    } else if (selection <= 8) {
        if (state.switch_locked[selection - 1]) {
            return;
        }
        http_publish_switch(selection - 1, !state.switch_state[selection - 1]);
    } else {
        int index = selection - 9, value = state.dimmer_val[index];
        if (value > 0) remembered_dimmer[index] = value;
        int target = value > 0 ? 0 : remembered_dimmer[index] ? remembered_dimmer[index] : 100;
        state.dimmer_hold_until[index] = millis() + 2000; http_publish_dimmer(index, target);
    }
    ui_update_home();
}
void card_click(lv_event_t *event) { ui_navigate_to((TabId)(uintptr_t)lv_event_get_user_data(event)); }
void link(lv_obj_t *card, TabId target) {
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, card_click, LV_EVENT_CLICKED, (void *)(uintptr_t)target);
}
void value(lv_obj_t *obj, const char *format, float number, bool ready) {
    if (ready && isfinite(number)) ui_label_set_float(obj, format, number);
    else ui_label_set_text_if_changed(obj, "--");
}
}
void ui_home_reset() {
    root = multiplus = mp_power = emer_btn = emer_bar = nullptr;
    mp_activity = HomeMpActivity();
    ui_victron_home_reset();
    if (emer_timer) { lv_timer_del(emer_timer); emer_timer = nullptr; }
    emer_press_start = 0; emer_triggered = false;
}
void ui_home_layout_changed() { layout(); }
void ui_update_home() {
    if (!root) return;
    if(update_mp_activity())layout();
    char text[96];
    bool ready = source_ready();
    for (int i = 0; i < 2; ++i) {
        int selection = state.home_favorite[i]; if (selection < 1 || selection > 18) continue;
        bool available = favorite_ready(selection);
        bool valid = available, pending = false;
        bool on;
        if (selection >= 17) {
            auto gx = victron_snapshot();
            valid = victron_fresh(gx, (GxField)(GX_RELAY0 + selection - 17));
            pending = gx.pending[selection - 16].active();
            on = gx.values[GX_RELAY0 + selection - 17].value == 1;
        } else {
            on = selection <= 8 ? state.switch_state[selection - 1] : state.dimmer_val[selection - 9] > 0;
        }
        const String &name = selection >= 17 ? state.victron_relay_names[selection - 17] :
            selection <= 8 ? state.switch_names[selection - 1] : state.dimmer_names[selection - 9];
        bool locked = (selection <= 8 && state.switch_locked[selection - 1]);
        if (locked) {
            snprintf(text, sizeof(text), MDI_LOCK " %s %s", name.c_str(), pending ? "Warte" : !valid ? "--" : on ? "EIN" : "AUS");
        } else {
            snprintf(text, sizeof(text), "%s %s", name.c_str(), pending ? "Warte" : !valid ? "--" : on ? "EIN" : "AUS");
        }
        ui_label_set_text_if_changed(favorite_text[i], text);
        ui_text_color_if_changed(favorite_text[i],valid&&on?ui_theme_btn_active_text():ui_theme_btn_text());
        if (available) lv_obj_clear_state(favorites[i], LV_STATE_DISABLED); else lv_obj_add_state(favorites[i], LV_STATE_DISABLED);
        if (valid && on) lv_obj_add_state(favorites[i], LV_STATE_CHECKED); else lv_obj_clear_state(favorites[i], LV_STATE_CHECKED);
    }
    for (int i = 0; i < 3; ++i) {
        int source = ui_home_temperature_source(i); if (source < 0 || source >= TEMP_SOURCE_COUNT) continue;
        ui_label_set_text_if_changed(temp_name[i], state.temp_sensor_names[source].c_str());
        // Dummy temp3 contains humidity; never mislabel it as Celsius.
        value(temp_value[i], state.temp_is_humidity[source] ? "%.1f %%" : "%.1f C",
              state.temp_sensors[source], ready && (state.temp_fields & (1 << source)));
    }
    if (ready && (state.battery_fields & 4)) {
        snprintf(text, sizeof(text), "Batterie %d%%", state.bat_soc);
    } else {
        snprintf(text, sizeof(text), "Batterie --%%");
    }
    ui_label_set_text_if_changed(bat_title, text);
    ui_text_color_if_changed(bat_title, ready && (state.battery_fields & 4) ?
        ui_battery_soc_color(state.bat_soc) : ui_theme_muted());
    value(bat_power, "%.1f W", state.bat_voltage * state.bat_current, ready && (state.battery_fields & 3) == 3);

    char v_buf[16], a_buf[16];
    if (ready && (state.battery_fields & 1)) snprintf(v_buf, sizeof(v_buf), "%.1f", state.bat_voltage);
    else strcpy(v_buf, "--");
    if (ready && (state.battery_fields & 2)) snprintf(a_buf, sizeof(a_buf), "%.1f", state.bat_current);
    else strcpy(a_buf, "--");
    snprintf(text, sizeof(text), "%s V   %s A", v_buf, a_buf);
    ui_label_set_text_if_changed(bat_detail, text);

    if (state.victron_starter) {
        auto gx=victron_snapshot();
        if(gx.demo==state.debug_mode && victron_fresh(gx,GX_STARTER))snprintf(v_buf,sizeof(v_buf),"%.1f",gx.values[GX_STARTER].value);
        else strcpy(v_buf,"--");
    } else if (ready && (state.battery_fields & 8)) snprintf(v_buf, sizeof(v_buf), "%.1f", state.starter_voltage);
    else strcpy(v_buf, "--");
    snprintf(text, sizeof(text), "Starter %s V", v_buf);
    ui_label_set_text_if_changed(starter, text);

    value(solar_power, "%.0f W", state.solar_power, ready && (state.solar_fields & 1));

    if (ready && (state.solar_fields & 4)) snprintf(v_buf, sizeof(v_buf), "%.1f", state.solar_voltage);
    else strcpy(v_buf, "--");
    if (ready && (state.solar_fields & 2)) snprintf(a_buf, sizeof(a_buf), "%.1f", state.solar_current);
    else strcpy(a_buf, "--");
    snprintf(text, sizeof(text), "%s V   %s A", v_buf, a_buf);
    ui_label_set_text_if_changed(solar_detail, text);
    int tank = constrain(state.home_water_source, 0, 3); ui_label_set_text_if_changed(water_title, state.tank_names[tank].c_str());
    bool tank_ready = ready && (state.tank_fields & (1 << tank));
    snprintf(text, sizeof(text), "%d%%", state.tank_level[tank]); ui_label_set_text_if_changed(water_value, tank_ready ? text : "--");
    int level = tank_ready ? state.tank_level[tank] : 0;
    if (lv_bar_get_value(water_bar) != level) lv_bar_set_value(water_bar, level, LV_ANIM_OFF);
    ui_update_victron_home();
}
void ui_build_home(lv_obj_t *parent) {
    root = parent;
    lv_obj_set_style_pad_all(parent, 8, 0); lv_obj_set_style_bg_color(parent, ui_theme_bg(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0); lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO); lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 2; ++i) {
        favorites[i] = lv_btn_create(parent); ui_favorite_button_style(favorites[i]);
        lv_obj_set_style_text_color(favorites[i], ui_theme_btn_active_text(), LV_STATE_CHECKED);
        lv_obj_add_event_cb(favorites[i], favorite_click, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        favorite_text[i] = label(favorites[i], "--", 0, 0, &lv_font_montserrat_16);
        lv_label_set_long_mode(favorite_text[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(favorite_text[i], LV_TEXT_ALIGN_CENTER, 0);
    }
    for (int i = 0; i < 3; ++i) {
        temperatures[i] = ui_create_card(parent, 149, 70);
        temp_name[i] = label(temperatures[i], "--", 0, 0, &lv_font_montserrat_12);
        lv_label_set_long_mode(temp_name[i], LV_LABEL_LONG_DOT);
        temp_value[i] = label(temperatures[i], "--", 0, 16, &lv_font_montserrat_22);
    }
    battery = ui_create_card(parent, 240, 122);
    bat_title = label(battery, "Batterie --%", 0, 0, &lv_font_montserrat_16);
    bat_power = label(battery, "--", 0, 25, &lv_font_montserrat_28);
    lv_obj_set_style_text_color(bat_power, lv_color_hex(UI_COLOR_WARNING), 0);
    bat_detail = label(battery, "-- V   -- A", 0, 58, &lv_font_montserrat_16);
    starter = label(battery, "Starter -- V", 0, 80, &lv_font_montserrat_12);
    solar = ui_create_card(parent, 216, 122); label(solar, "Solar", 0, 0, &lv_font_montserrat_16);
    solar_power = label(solar, "--", 0, 25, &lv_font_montserrat_28);
    lv_obj_set_style_text_color(solar_power, lv_color_hex(UI_COLOR_WARNING), 0);
    solar_detail = label(solar, "-- V   -- A", 0, 58, &lv_font_montserrat_16);
    label(solar, "PV-Leistung", 0, 80, &lv_font_montserrat_12);
    water = ui_create_card(parent, 464, 60);
    water_title = label(water, "--", 0, 0, &lv_font_montserrat_16);
    water_value = label(water, "--", 360, 0, &lv_font_montserrat_22);
    water_bar = lv_bar_create(water); lv_obj_set_pos(water_bar, 0, 26); lv_obj_set_size(water_bar, 438, 8);
    lv_bar_set_range(water_bar, 0, 100); lv_obj_set_style_bg_color(water_bar, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(water_bar, ui_theme_tank(), LV_PART_INDICATOR);
    for (auto card : temperatures) link(card, state.tab_enabled[TAB_TEMPS] ? TAB_TEMPS : (state.tab_enabled[TAB_TRUMA] ? TAB_TRUMA : TAB_CLIMATE));
    link(battery, TAB_POWER); link(solar, TAB_POWER); link(water, TAB_WATER);

    // Notfallknopf (5s halten zum Auslösen)
    emer_btn = lv_btn_create(parent);
    lv_obj_set_size(emer_btn, 464, 52);
    lv_obj_set_style_bg_color(emer_btn, lv_color_hex(UI_COLOR_DANGER), 0);
    lv_obj_set_style_bg_color(emer_btn, lv_color_hex(0xAA0000), LV_STATE_PRESSED);
    lv_obj_set_style_radius(emer_btn, ui_radius_button(), 0);
    lv_obj_set_style_border_width(emer_btn, 0, 0);
    lv_obj_t *emer_lbl = lv_label_create(emer_btn);
    lv_label_set_text(emer_lbl, LV_SYMBOL_WARNING " NOTFALL — 5 Sek. halten");
    lv_obj_set_style_text_font(emer_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(emer_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(emer_lbl, LV_ALIGN_CENTER, 0, -8);
    emer_bar = lv_bar_create(emer_btn);
    lv_obj_set_size(emer_bar, 430, 6);
    lv_obj_align(emer_bar, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_bar_set_range(emer_bar, 0, 100);
    lv_bar_set_value(emer_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(emer_bar, lv_color_hex(0x880000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(emer_bar, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    lv_obj_set_style_radius(emer_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(emer_bar, 3, LV_PART_INDICATOR);
    if (!emer_timer) emer_timer = lv_timer_create(emer_timer_cb, 50, nullptr);

    update_mp_activity();layout();
}
