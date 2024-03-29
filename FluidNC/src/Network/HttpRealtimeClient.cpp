#include "../Config.h"

#define RETRY -1

#    include <lwip/sockets.h>

// A workaround for an issue in lwip/sockets.h
// See https://github.com/espressif/arduino-esp32/issues/4073
#    undef IPADDR_NONE

#    include "../System.h"
#    include "HttpRealtimeClient.h"

HttpRealtimeClient::HttpRealtimeClient(const char *name, WiFiClient wifi_client) :
    HttpBatchClient(name, wifi_client) {
}

HttpRealtimeClient::~HttpRealtimeClient() {
}

Channel* HttpRealtimeClient::pollLine(char* line) {
  int code;
  while (code = read(), code != RETRY) {
    if (is_realtime_command(code)) {
      handleRealtimeCharacter((uint8_t)code);
    }
  }
  return nullptr;
}
