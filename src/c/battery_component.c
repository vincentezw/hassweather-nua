#include "battery_component.h"

static BatteryChargeState s_battery_state;
static void (*s_change_callback)(void) = NULL;

static void battery_callback(BatteryChargeState state) {
  s_battery_state = state;
  if (s_change_callback) {
    s_change_callback();
  }
}

void battery_component_init(void (*on_change_cb)(void)) {
  s_change_callback = on_change_cb;
  battery_state_service_subscribe(battery_callback);
  s_battery_state = battery_state_service_peek();
}

void battery_component_deinit(void) {
  battery_state_service_unsubscribe();
}

BatteryChargeState battery_component_get_state(void) {
  return s_battery_state;
}

void draw_battery_icon(GContext *ctx, GPoint origin, uint8_t charge_percent, bool is_charging) {
  int width = 22;
  int height = 11;

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, GRect(origin.x, origin.y, width, height));
  graphics_draw_line(ctx, GPoint(origin.x + width, origin.y + 3), GPoint(origin.x + width, origin.y + 7));

  GColor fill_color;
  if (is_charging) {
    fill_color = COLOR_FALLBACK(GColorYellow, GColorBlack);
  } else if (charge_percent <= 20) {
    fill_color = COLOR_FALLBACK(GColorRed, GColorBlack);
  } else {
    fill_color = GColorBlack;
  }

  int max_fill_width = width - 4;
  int fill_width = (max_fill_width * charge_percent) / 100;

  if (charge_percent > 0 && fill_width == 0) {
    fill_width = 1;
  }

  if (fill_width > 0) {
    graphics_context_set_fill_color(ctx, fill_color);
    graphics_fill_rect(ctx, GRect(origin.x + 2, origin.y + 2, fill_width, height - 4), 0, GCornerNone);
  }

  static char battery_buffer[8];
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", charge_percent);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, battery_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(origin.x - 4, origin.y + 10, 32, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}
