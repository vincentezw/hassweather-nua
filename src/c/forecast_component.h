#pragma once
#include <pebble.h>
#include "hassweather-nua.h"

#define DISPLAY_SLOTS 4
#define WEATHER_ICON_COUNT 9

typedef struct {
  TextLayer *hour_layer;
  BitmapLayer *icon_layer;
  TextLayer *temp_layer;
  GBitmap *icon;
} WeatherSlot;

void forecast_component_init(void);
void forecast_component_deinit(void);
void forecast_component_create_slots(Layer *parent, GRect bounds);
void forecast_component_destroy_slots(void);
void forecast_component_parse_data(Tuple *tuple);
void forecast_component_render(void);
