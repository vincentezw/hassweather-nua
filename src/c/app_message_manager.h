#pragma once
#include <pebble.h>
#define COOLDOWN_SECONDS 15       // Minimum time between outgoing requests
#define REQUEST_TIMEOUT_MS 10000  // Reset busy state after 10s if no response arrives

// Commands
typedef enum {
  CMD_DATA = 0,
} MessageCommand;

// Callback when a message is received.
typedef void (*AppMessageCallback)(
  MessageCommand command,
  Tuple *forecast,
  Tuple *sunData
);

void app_message_manager_init(AppMessageCallback callback);
void app_message_manager_deinit(void);

AppMessageResult app_message_send_command(MessageCommand command);

