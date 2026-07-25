#pragma once
#include <pebble.h>

void battery_component_init(void (*on_change_cb)(void));
void battery_component_deinit(void);
BatteryChargeState battery_component_get_state(void);
void draw_battery_icon(GContext *ctx, GPoint origin, uint8_t charge_percent, bool is_charging);
