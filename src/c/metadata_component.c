#include "metadata_component.h"
#include "battery_component.h"
#include "pebble.h"

static Layer *s_metadata_layer;
static GDrawCommandImage *s_date_image;
static GBitmap *s_disconnected_icon;
static bool s_is_connected = true;

static void bluetooth_callback(bool connected) {
  s_is_connected = connected;

  if (s_metadata_layer) {
    layer_mark_dirty(s_metadata_layer);
  }
}

static void metadata_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  if (s_date_image) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    char currentDayNum[5];
    strftime(currentDayNum, 3, "%e", tick_time);
    if (currentDayNum[0] == ' ') {
      currentDayNum[0] = currentDayNum[1];
      currentDayNum[1] = '\0';
    }

    gdraw_command_image_draw(ctx, s_date_image, GPoint(bounds.size.w - 36, 20));
    graphics_context_set_text_color(ctx, GColorBlack);
    char * font_key = FONT_KEY_GOTHIC_18_BOLD;
    int font_x = bounds.size.w - 37;
    int font_y = 23;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    font_key = FONT_KEY_GOTHIC_24_BOLD;
    font_x = bounds.size.w - 35;
    font_y = 20;
#endif
    graphics_draw_text(ctx, currentDayNum, fonts_get_system_font(font_key),
                       GRect(font_x, font_y, 26, 26),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  BatteryChargeState b_state = battery_component_get_state();
  GPoint battery_pos = GPoint(bounds.size.w - 74, 23);
  draw_battery_icon(ctx, battery_pos, b_state.charge_percent, b_state.is_charging);

  if (!s_is_connected && s_disconnected_icon) {
    GRect icon_bounds = gbitmap_get_bounds(s_disconnected_icon);
    int x = bounds.size.w - 108;
    int y = 23;

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_disconnected_icon, GRect(x, y, icon_bounds.size.w, icon_bounds.size.h));
  }
}

void metadata_component_init(GDrawCommandImage *date_img, GBitmap *disconnected_icon) {
  s_date_image = date_img;
  s_disconnected_icon = disconnected_icon;

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  s_is_connected = connection_service_peek_pebble_app_connection();
}

void metadata_component_create_layer(GRect bounds) {
  s_metadata_layer = layer_create(bounds);
  layer_set_update_proc(s_metadata_layer, metadata_layer_update_proc);
}

Layer* metadata_component_get_layer(void) {
  return s_metadata_layer;
}

void metadata_component_destroy_layer(void) {
  if (s_metadata_layer) {
    layer_destroy(s_metadata_layer);
    s_metadata_layer = NULL;
  }
}
