#include "wled_client.h"
#include "system_state.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

static void wled_post(const String &body) {
    if (state.wled_ip.length() == 0) return;
    HTTPClient http;
    String url = "http://" + state.wled_ip + "/json/state";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.POST(body);
    http.end();
}

void wled_set_power(bool on) {
    wled_post(on ? "{\"on\":true}" : "{\"on\":false}");
}

void wled_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    char buf[72];
    snprintf(buf, sizeof(buf), "{\"on\":true,\"seg\":[{\"col\":[[%d,%d,%d,%d]]}]}", r, g, b, w);
    wled_post(buf);
}

void wled_set_brightness(uint8_t bri) {
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"bri\":%d}", bri);
    wled_post(buf);
}

void wled_set_white(uint8_t w) {
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"seg\":[{\"col\":[[0,0,0,%d]]}]}", w);
    wled_post(buf);
}

void wled_set_state(bool on, uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t bri) {
    char buf[104];
    snprintf(buf, sizeof(buf),
        "{\"on\":%s,\"bri\":%d,\"seg\":[{\"col\":[[%d,%d,%d,%d]]}]}",
        on ? "true" : "false", bri, r, g, b, w);
    wled_post(buf);
}

// ─── dynamic list helpers ─────────────────────────────────────────────────────

static String wled_get(const String &path) {
    if (state.wled_ip.length() == 0) return "";
    HTTPClient http;
    http.begin("http://" + state.wled_ip + path);
    http.setTimeout(3000);
    int code = http.GET();
    String body = (code == 200) ? http.getString() : "";
    http.end();
    return body;
}

void wled_fetch_effects(void) {
    String body = wled_get("/json/effects");
    if (body.length() == 0) return;
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) return;
    JsonArray arr = doc.as<JsonArray>();
    state.wled_effects_opts = "";
    state.wled_fx_count = 0;
    for (JsonVariant v : arr) {
        if (state.wled_fx_count > 0) state.wled_effects_opts += "\n";
        state.wled_effects_opts += v.as<String>();
        state.wled_fx_count++;
        if (state.wled_fx_count >= 100) break;
    }
    state.wled_effects_loaded = (state.wled_fx_count > 0);
}

void wled_fetch_presets(void) {
    String body = wled_get("/json/presets");
    if (body.length() == 0) return;
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) return;
    JsonObject obj = doc.as<JsonObject>();
    state.wled_presets_opts = "";
    state.wled_preset_count = 0;
    for (JsonPair kv : obj) {
        int id = String(kv.key().c_str()).toInt();
        if (id <= 0) continue;
        const char *name = kv.value()["n"] | "";
        if (strlen(name) == 0) continue;
        if (state.wled_preset_count > 0) state.wled_presets_opts += "\n";
        state.wled_presets_opts += name;
        if (state.wled_preset_count < 32) state.wled_preset_ids[state.wled_preset_count] = id;
        state.wled_preset_count++;
        if (state.wled_preset_count >= 32) break;
    }
    state.wled_presets_loaded = (state.wled_preset_count > 0);
}

void wled_set_effect(int fx_id) {
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"on\":true,\"seg\":[{\"fx\":%d}]}", fx_id);
    wled_post(buf);
}

void wled_set_preset(int preset_id) {
    char buf[24];
    snprintf(buf, sizeof(buf), "{\"ps\":%d}", preset_id);
    wled_post(buf);
}
