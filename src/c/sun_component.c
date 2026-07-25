#include "sun_component.h"

static SunData s_sun = {0};
static SunState s_current_sun_state = {0};
static Layer *s_sunstate_background_layer;
static TextLayer *s_sun_time_layer;
static GBitmap *s_sunrise_icon;
static GBitmap *s_sunset_icon;
static char s_sun_time_buffer[8];

static void update_sun_state(void) {
  SunState state = {0};

  if (s_sun.sunrise == 0 || s_sun.sunset == 0) {
    state.is_valid = false;
    s_current_sun_state = state;
    return;
  }

  time_t now = time(NULL);
  const time_t DAY_SECONDS = 86400;

  if (s_sun.sunrise <= now && s_sun.sunset <= now) {
    state.is_valid = false;
    s_current_sun_state = state;
    return;
  }

  state.is_valid = true;
  if ((s_sun.sunset < s_sun.sunrise) || s_sun.sunrise <= now) {
    state.is_daytime = true;
    state.end = s_sun.sunset;
    state.start = (s_sun.sunrise <= now) ? s_sun.sunrise : (state.end - DAY_SECONDS);
  } else {
    state.is_daytime = false;
    state.end = s_sun.sunrise;
    state.start = (s_sun.sunset <= now) ? s_sun.sunset : (s_sun.sunset - DAY_SECONDS);
  }

  state.next_event_time = state.end;
  s_current_sun_state = state;
}

static void sunstate_background_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  if (!s_current_sun_state.is_valid) {
    graphics_context_set_fill_color(ctx, GColorLightGray);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    return;
  }

  time_t now = time(NULL);
  float progress = (s_current_sun_state.end > s_current_sun_state.start)
    ? (float)(now - s_current_sun_state.start) / (float)(s_current_sun_state.end - s_current_sun_state.start)
    : 0.0f;

  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;

  graphics_context_set_fill_color(ctx, s_current_sun_state.is_daytime ? GColorYellow : GColorVividCerulean);
  graphics_fill_rect(ctx, GRect(0, 0, (int)(bounds.size.w * progress), bounds.size.h), 0, GCornerNone);

  GBitmap *icon = s_current_sun_state.is_daytime ? s_sunset_icon : s_sunrise_icon;
  if (icon) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    GRect icon_bounds = gbitmap_get_bounds(icon);
    int icon_y = (bounds.size.h - icon_bounds.size.h) / 2;
    graphics_draw_bitmap_in_rect(ctx, icon, GRect(4, icon_y, icon_bounds.size.w, icon_bounds.size.h));
  }
}

void sun_component_init(GBitmap *sunrise_icon, GBitmap *sunset_icon) {
  s_sunrise_icon = sunrise_icon;
  s_sunset_icon = sunset_icon;
}

void sun_component_create_layer(GRect bounds) {
  s_sunstate_background_layer = layer_create(GRect(0, 0, bounds.size.w, 20));
  layer_set_update_proc(s_sunstate_background_layer, sunstate_background_draw);

  s_sun_time_layer = text_layer_create(GRect(30, -3, bounds.size.w - 24, 18));
  text_layer_set_background_color(s_sun_time_layer, GColorClear);
  text_layer_set_text_color(s_sun_time_layer, GColorBlack);
  text_layer_set_font(s_sun_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_sun_time_layer, GTextAlignmentLeft);

  layer_add_child(s_sunstate_background_layer, text_layer_get_layer(s_sun_time_layer));
}

Layer* sun_component_get_background_layer(void) {
  return s_sunstate_background_layer;
}

void sun_component_destroy_layer(void) {
  if (s_sun_time_layer) text_layer_destroy(s_sun_time_layer);
  if (s_sunstate_background_layer) layer_destroy(s_sunstate_background_layer);
}

void sun_component_parse_data(Tuple *tuple) {
  if (!tuple || tuple->type != TUPLE_CSTRING || tuple->length == 0) return;
  char *str = (char *)tuple->value;
  s_sun.sunrise = (time_t)atol(str);

  char *comma = strchr(str, ',');
  if (comma) {
    s_sun.sunset = (time_t)atol(comma + 1);
  }
}

void sun_component_render(void) {
  update_sun_state();

  if (s_current_sun_state.is_valid) {
    struct tm *target_tm = localtime(&s_current_sun_state.next_event_time);

    if (clock_is_24h_style()) {
      strftime(s_sun_time_buffer, sizeof(s_sun_time_buffer), "%H:%M", target_tm);
    } else {
      strftime(s_sun_time_buffer, sizeof(s_sun_time_buffer), "%I:%M", target_tm);
      if (s_sun_time_buffer[0] == '0') {
        memmove(s_sun_time_buffer, s_sun_time_buffer + 1, strlen(s_sun_time_buffer));
      }
    }

    text_layer_set_text(s_sun_time_layer, s_sun_time_buffer);
  } else {
    text_layer_set_text(s_sun_time_layer, "No Sun Data");
  }

  if (s_sunstate_background_layer) {
    layer_mark_dirty(s_sunstate_background_layer);
  }
}

bool sun_component_is_event_due(time_t now) {
  return s_current_sun_state.is_valid && (now >= s_current_sun_state.next_event_time);
}

bool sun_component_is_valid(void) {
  return s_current_sun_state.is_valid;
}
