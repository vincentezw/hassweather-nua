#pragma once
#include <pebble.h>
#include "hassweather-nua.h"

void sun_component_init(GBitmap *sunrise_icon, GBitmap *sunset_icon);
void sun_component_deinit(void);
void sun_component_create_layer(GRect bounds);
void sun_component_destroy_layer(void);
Layer* sun_component_get_background_layer(void);

void sun_component_parse_data(Tuple *tuple);
void sun_component_render(void);
bool sun_component_is_event_due(time_t now);
bool sun_component_is_valid(void);
