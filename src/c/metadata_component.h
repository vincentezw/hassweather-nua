#pragma once
#include <pebble.h>

void metadata_component_init(GDrawCommandImage *date_img, GBitmap *disconnected_icon);
void metadata_component_deinit(void);
Layer* metadata_component_get_layer(void);
void metadata_component_create_layer(GRect bounds);
void metadata_component_destroy_layer(void);
