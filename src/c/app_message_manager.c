#include <pebble.h>
#include "app_message_manager.h"

static AppMessageCallback s_callback = NULL;
static bool s_waiting_for_reply = false;
static time_t s_last_request_time = 0;
static AppTimer *s_timeout_timer = NULL;

static void reset_busy_state(void) {
  s_waiting_for_reply = false;
  if (s_timeout_timer) {
    app_timer_cancel(s_timeout_timer);
    s_timeout_timer = NULL;
  }
}

static void request_timeout_callback(void *data) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Request timed out without a reply. Unlocking.");
  s_timeout_timer = NULL;
  s_waiting_for_reply = false;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  // Clear the in-flight lock as soon as any payload arrives
  reset_busy_state();

  Tuple *cmd = dict_find(iter, MESSAGE_KEY_COMMAND);
  if (!cmd || !s_callback) {
    return;
  }

  Tuple *forecast = dict_find(iter, MESSAGE_KEY_FORECAST);
  Tuple *sunData = dict_find(iter, MESSAGE_KEY_SUN);
  
  s_callback(
    (MessageCommand)cmd->value->uint8,
    forecast,
    sunData
  );
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Inbox dropped: %d", reason);
  // Optional: Do not unlock on inbox dropped unless you're sure it was the expected packet
}

static void outbox_sent(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message successfully sent to phone. Waiting for inbox reply...");
}

static void outbox_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox failed to send: %d", reason);
  // Phone isn't reachable or Bluetooth is disconnected. Release the lock immediately.
  reset_busy_state();
}

void app_message_manager_init(AppMessageCallback callback) {
  s_callback = callback;

  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_register_outbox_sent(outbox_sent);
  app_message_register_outbox_failed(outbox_failed);

  app_message_open(512, 512);
}

void app_message_manager_deinit(void) {
  reset_busy_state();
}

AppMessageResult app_message_send_command(MessageCommand command) {
  time_t now = time(NULL);

  // 1. Guard against in-flight requests
  if (s_waiting_for_reply) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Request skipped: Already waiting for a response from phone.");
    return APP_MSG_BUSY;
  }

  // 2. Guard against time-based rate limiting (cooldown)
  if (now - s_last_request_time < COOLDOWN_SECONDS) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Request skipped: Cooldown active (%ld/%d sec).", 
            now - s_last_request_time, COOLDOWN_SECONDS);
    return APP_MSG_BUSY;
  }

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox begin failed: %d", result);
    return result;
  }

  dict_write_uint8(iter, MESSAGE_KEY_COMMAND, command);
  dict_write_end(iter);

  result = app_message_outbox_send();
  if (result == APP_MSG_OK) {
    s_waiting_for_reply = true;
    s_last_request_time = now;

    // Start safety timeout in case the phone drops or ignores the request
    s_timeout_timer = app_timer_register(REQUEST_TIMEOUT_MS, request_timeout_callback, NULL);
  }

  return result;
}
