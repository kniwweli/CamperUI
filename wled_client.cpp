#include "wled_client.h"
#include "system_state.h"
#include <HTTPClient.h>

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
