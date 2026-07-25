#include "forecast_component.h"
#include "app_message_manager.h"

static ForecastData s_forecast = {0};
static GBitmap *s_weather_icons[WEATHER_ICON_COUNT];
static WeatherSlot s_weather_slots[DISPLAY_SLOTS];
static const int k_display_offsets[DISPLAY_SLOTS] = {0, 1, 3, 5};

static const uint32_t WEATHER_ICON_RESOURCES[] = {
  RESOURCE_ID_WEATHER_SUNNY,
  RESOURCE_ID_WEATHER_PARTLY_CLOUDY,
  RESOURCE_ID_WEATHER_CLOUDY,
  RESOURCE_ID_WEATHER_LIGHT_RAIN,
  RESOURCE_ID_WEATHER_HEAVY_RAIN,
  RESOURCE_ID_WEATHER_HEAVY_SNOW,
  RESOURCE_ID_WEATHER_RAIN_SNOW,
  RESOURCE_ID_WEATHER_GENERIC,
  RESOURCE_ID_WEATHER_UNKNOWN
};

static char* next_token(char **cursor) {
  if (*cursor == NULL) return NULL;
  char *start = *cursor;
  char *comma = strchr(start, ',');
  if (comma) {
    *comma = '\0';
    *cursor = comma + 1;
  } else {
    *cursor = NULL;
  }
  return start;
}

void forecast_component_init(void) {
  for (int i = 0; i < WEATHER_ICON_COUNT; i++) {
    s_weather_icons[i] = gbitmap_create_with_resource(WEATHER_ICON_RESOURCES[i]);
  }
}

void forecast_component_deinit(void) {
  for (int i = 0; i < WEATHER_ICON_COUNT; i++) {
    if (s_weather_icons[i]) {
      gbitmap_destroy(s_weather_icons[i]);
      s_weather_icons[i] = NULL;
    }
  }
}

void forecast_component_create_slots(Layer *parent, GRect bounds) {
  const int icon_size = 40;
  const int label_height = 18;
  const int bottom_margin = 10;
  const int spacing = (bounds.size.w - (DISPLAY_SLOTS * icon_size)) / (DISPLAY_SLOTS + 1);

  for (int i = 0; i < DISPLAY_SLOTS; i++) {
    int x = spacing + i * (icon_size + spacing);

    // Hour Layer
    s_weather_slots[i].hour_layer = text_layer_create(
      GRect(x, bounds.size.h - bottom_margin - icon_size - label_height * 2 - 4, icon_size, label_height)
    );
    text_layer_set_text(s_weather_slots[i].hour_layer, "");
    text_layer_set_text_alignment(s_weather_slots[i].hour_layer, GTextAlignmentCenter);
    text_layer_set_font(s_weather_slots[i].hour_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_background_color(s_weather_slots[i].hour_layer, GColorClear);
    layer_add_child(parent, text_layer_get_layer(s_weather_slots[i].hour_layer));

    // Icon Layer
    GRect icon_frame = GRect(x, bounds.size.h - bottom_margin - icon_size - label_height - 2, icon_size, icon_size);
    s_weather_slots[i].icon_layer = bitmap_layer_create(icon_frame);
    bitmap_layer_set_background_color(s_weather_slots[i].icon_layer, GColorClear);
    bitmap_layer_set_compositing_mode(s_weather_slots[i].icon_layer, GCompOpSet);
    s_weather_slots[i].icon = s_weather_icons[8]; // Default to unknown icon
    bitmap_layer_set_bitmap(s_weather_slots[i].icon_layer, s_weather_slots[i].icon);
    layer_add_child(parent, bitmap_layer_get_layer(s_weather_slots[i].icon_layer));

    // Temperature Layer
    s_weather_slots[i].temp_layer = text_layer_create(
      GRect(x, bounds.size.h - bottom_margin - label_height, icon_size, label_height)
    );
    text_layer_set_text(s_weather_slots[i].temp_layer, "");
    text_layer_set_text_alignment(s_weather_slots[i].temp_layer, GTextAlignmentCenter);
    text_layer_set_background_color(s_weather_slots[i].temp_layer, GColorClear);
    layer_add_child(parent, text_layer_get_layer(s_weather_slots[i].temp_layer));
  }
}

void forecast_component_destroy_slots(void) {
  for (int i = 0; i < DISPLAY_SLOTS; i++) {
    if (s_weather_slots[i].icon_layer) {
      bitmap_layer_destroy(s_weather_slots[i].icon_layer);
      s_weather_slots[i].icon_layer = NULL;
    }
    if (s_weather_slots[i].hour_layer) {
      text_layer_destroy(s_weather_slots[i].hour_layer);
      s_weather_slots[i].hour_layer = NULL;
    }
    if (s_weather_slots[i].temp_layer) {
      text_layer_destroy(s_weather_slots[i].temp_layer);
      s_weather_slots[i].temp_layer = NULL;
    }
  }
}

void forecast_component_parse_data(Tuple *tuple) {
  if (!tuple) return;
  char buffer[128];
  strncpy(buffer, tuple->value->cstring, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';

  char *cursor = buffer;
  char *token = next_token(&cursor);
  if (!token) return;

  s_forecast.timestamp = atol(token);

  for (int i = 0; i < FORECAST_HOURS; i++) {
    token = next_token(&cursor);
    if (!token) return;
    s_forecast.condition[i] = atoi(token);

    token = next_token(&cursor);
    if (!token) return;
    s_forecast.temperature[i] = atoi(token);
  }
}

void forecast_component_render(void) {
  if (s_forecast.timestamp == 0) return;

  time_t now = time(NULL);
  double diff_seconds = difftime(now, s_forecast.timestamp);
  int current_index = (int)(diff_seconds / 3600);

  // Expired forecast guard
  if (current_index < 0 || current_index >= FORECAST_HOURS) {
    for (int slot = 0; slot < DISPLAY_SLOTS; slot++) {
      text_layer_set_text(s_weather_slots[slot].hour_layer, "");
      text_layer_set_text(s_weather_slots[slot].temp_layer, "");
      bitmap_layer_set_bitmap(s_weather_slots[slot].icon_layer, s_weather_icons[8]);
    }
    app_message_send_command(CMD_DATA);
    return;
  }

  for (int slot = 0; slot < DISPLAY_SLOTS; slot++) {
    int forecast_index = current_index + k_display_offsets[slot];

    if (forecast_index >= FORECAST_HOURS) {
      text_layer_set_text(s_weather_slots[slot].hour_layer, "");
      text_layer_set_text(s_weather_slots[slot].temp_layer, "");
      bitmap_layer_set_bitmap(s_weather_slots[slot].icon_layer, s_weather_icons[8]);
      continue;
    }

    time_t hour_time = s_forecast.timestamp + (forecast_index * 3600);
    struct tm *tm_info = localtime(&hour_time);
    static char hour_buffers[DISPLAY_SLOTS][8];

    if (slot == 0) {
      snprintf(hour_buffers[slot], sizeof(hour_buffers[slot]), "Now");
    } else if (clock_is_24h_style()) {
      strftime(hour_buffers[slot], sizeof(hour_buffers[slot]), "%Hh", tm_info);
    } else {
      strftime(hour_buffers[slot], sizeof(hour_buffers[slot]), "%I%P", tm_info);
      if (hour_buffers[slot][0] == '0') {
        memmove(hour_buffers[slot], hour_buffers[slot] + 1, strlen(hour_buffers[slot]));
      }
    }

    text_layer_set_text(s_weather_slots[slot].hour_layer, hour_buffers[slot]);

    static char temp_buffers[DISPLAY_SLOTS][8];
    snprintf(temp_buffers[slot], sizeof(temp_buffers[slot]), "%d°", s_forecast.temperature[forecast_index]);
    text_layer_set_text(s_weather_slots[slot].temp_layer, temp_buffers[slot]);
    bitmap_layer_set_bitmap(s_weather_slots[slot].icon_layer, s_weather_icons[s_forecast.condition[forecast_index]]);
  }
}
