#include <pebble.h>
#include <pebble-fctx/ffont.h>
#include "app_message_manager.h"
#include "hassweather-nua.h"
#include "battery_component.h"
#include "metadata_component.h"
#include "sun_component.h"
#include "forecast_component.h"


static Window *s_window;
static Layer *s_time_layer;
static Layer *s_weather_background_layer;

static GBitmap *s_sunrise_icon;
static GBitmap *s_sunset_icon;
static GBitmap *s_bt_disconnected_icon;
static GDrawCommandImage *s_date_image;
static FFont *s_avenir_font;
static GColor s_current_bg_color;

static const GColor WEATHER_BACKGROUNDS[] = {
  GColorCyan, GColorMintGreen, GColorYellow, GColorOrange,
  GColorMagenta, GColorMelon, GColorCeleste, GColorVividCerulean
};
#define WEATHER_BG_COUNT (sizeof(WEATHER_BACKGROUNDS) / sizeof(WEATHER_BACKGROUNDS[0]))

static void pick_next_bg_color(void) {
  uint32_t new_index = rand() % WEATHER_BG_COUNT;
  
  if (gcolor_equal(WEATHER_BACKGROUNDS[new_index], s_current_bg_color)) {
    new_index = (new_index + 1) % WEATHER_BG_COUNT;
  }

  s_current_bg_color = WEATHER_BACKGROUNDS[new_index];
}

static void weather_background_draw(Layer *layer, GContext *ctx) {
  GColor fill_color = COLOR_FALLBACK(s_current_bg_color, GColorLightGray);
  graphics_context_set_fill_color(ctx, fill_color);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void time_layer_update_proc(Layer *layer, GContext *ctx) {
  if (!s_avenir_font) return;

  GRect bounds = layer_get_bounds(layer);
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_time_buffer[8];

  strftime(s_time_buffer, sizeof(s_time_buffer), 
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  if (!clock_is_24h_style() && s_time_buffer[0] == '0') {
    memmove(s_time_buffer, s_time_buffer + 1, strlen(s_time_buffer));
  }

  FContext fctx;
  fctx_init_context(&fctx, ctx);
  fctx_set_color_bias(&fctx, 0);
  fctx_set_fill_color(&fctx, GColorBlack);
  int font_size = 52;
  int time_y = 42;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  font_size = 78;
  time_y = 60;
#endif
  fctx_set_text_em_height(&fctx, s_avenir_font, font_size);

  fctx_set_offset(&fctx, FPointI(bounds.size.w / 2, time_y));
  fctx_begin_fill(&fctx);
  fctx_draw_string(&fctx, s_time_buffer, s_avenir_font, GTextAlignmentCenter, FTextAnchorMiddle);
  fctx_end_fill(&fctx);

  fctx_deinit_context(&fctx);
}

static void on_battery_changed(void) {
  Layer *metadata_layer = metadata_component_get_layer();
  if (metadata_layer) {
    layer_mark_dirty(metadata_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (s_time_layer) {
    layer_mark_dirty(s_time_layer);
  }

  time_t now = time(NULL);
  if (sun_component_is_event_due(now) || (tick_time->tm_min % 5 == 0)) {
    sun_component_render();
  }

  if (tick_time->tm_min == 0) {
    forecast_component_render();

    Layer *metadata_layer = metadata_component_get_layer();
    if (metadata_layer) {
      layer_mark_dirty(metadata_layer);
    }
  }

  if (tick_time->tm_min == 0 || !sun_component_is_valid()) {
    app_message_send_command(CMD_DATA);
  }
}

static void message_received(MessageCommand command, Tuple *forecast, Tuple *sunData) {
  forecast_component_parse_data(forecast);
  sun_component_parse_data(sunData);
  pick_next_bg_color();

  if (s_weather_background_layer) {
    layer_mark_dirty(s_weather_background_layer);
  }

  forecast_component_render();
  sun_component_render();
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 1. Sun State Layer
  sun_component_create_layer(bounds);
  layer_add_child(window_layer, sun_component_get_background_layer());

  // 2. Main Time Layer
  s_time_layer = layer_create(GRect(0, 25, bounds.size.w, bounds.size.h - 170));
  layer_set_update_proc(s_time_layer, time_layer_update_proc);
  layer_add_child(window_layer, s_time_layer);

  // 3. Weather Background & Slots
  int weather_height = 75;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  weather_height = 90;
#endif
  s_weather_background_layer = layer_create(GRect(0, bounds.size.h - weather_height, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_weather_background_layer, weather_background_draw);
  layer_add_child(window_layer, s_weather_background_layer);
  forecast_component_create_slots(window_layer, bounds);

  // 4. Metadata Layer (Date & Battery)
  int metadata_y = 43;
  int offset_y = 15;
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  metadata_y = 80;
  offset_y = 60;
#endif
  metadata_component_create_layer(GRect(0, metadata_y, bounds.size.w, bounds.size.h - offset_y));
  layer_add_child(window_layer, metadata_component_get_layer());

  // Initial renders
  sun_component_render();
  forecast_component_render();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_time_layer);
  layer_destroy(s_weather_background_layer);

  sun_component_destroy_layer();
  forecast_component_destroy_slots();
  metadata_component_destroy_layer();
}

static void prv_init(void) {
  s_current_bg_color = WEATHER_BACKGROUNDS[rand() % WEATHER_BG_COUNT];

  s_bt_disconnected_icon = gbitmap_create_with_resource(RESOURCE_ID_BT_DISCONNECTED);
  s_sunrise_icon = gbitmap_create_with_resource(RESOURCE_ID_SUNRISE);
  s_sunset_icon = gbitmap_create_with_resource(RESOURCE_ID_SUNSET);
  s_avenir_font = ffont_create_from_resource(RESOURCE_ID_AVENIR_NEXT_REGULAR);

  s_date_image = gdraw_command_image_create_with_resource(RESOURCE_ID_DATE_BG);
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
  s_date_image = gdraw_command_image_create_with_resource(RESOURCE_ID_DATE_BG_LG);
#endif

  battery_component_init(on_battery_changed);
  metadata_component_init(s_date_image, s_bt_disconnected_icon);
  sun_component_init(s_sunrise_icon, s_sunset_icon);
  forecast_component_init();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  window_stack_push(s_window, true);
  app_message_manager_init(message_received);
}

static void prv_deinit(void) {
  window_destroy(s_window);

  if (s_bt_disconnected_icon) gbitmap_destroy(s_bt_disconnected_icon);
  if (s_sunrise_icon) gbitmap_destroy(s_sunrise_icon);
  if (s_sunset_icon) gbitmap_destroy(s_sunset_icon);
  if (s_avenir_font) ffont_destroy(s_avenir_font);
  if (s_date_image) gdraw_command_image_destroy(s_date_image);

  battery_component_deinit();
  forecast_component_deinit();
}

int main(void) {
  prv_init();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_event_loop();
  prv_deinit();
}
