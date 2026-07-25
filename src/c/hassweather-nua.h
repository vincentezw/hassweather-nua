#pragma once
#define NUM_ICONS 4
#define FORECAST_HOURS 8
#define FORECAST_PERSIST_KEY 100
#define SUN_PERSIST_KEY 101

typedef struct {
  BitmapLayer *icon_layer;
  GBitmap *icon;

  TextLayer *hour_layer;
  TextLayer *temp_layer;
} WeatherIconSlot;

typedef struct {
  uint32_t timestamp;
  uint8_t condition[FORECAST_HOURS];
  int8_t temperature[FORECAST_HOURS];
} ForecastData;

typedef struct {
  time_t sunrise;
  time_t sunset;
} SunData;

typedef struct {
  bool is_daytime;
  time_t start;
  time_t end;
  time_t next_event_time;
  bool is_valid;
} SunState;
