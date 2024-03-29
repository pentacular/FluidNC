#include "../Config.h"
#include "../InputFile.h"
#include "../Machine/MachineConfig.h"
#include "../Planner.h"
#include "../SettingsDefinitions.h"
#include "../Types.h"

#define RETRY -1

#    include <lwip/sockets.h>

// A workaround for an issue in lwip/sockets.h
// See https://github.com/espressif/arduino-esp32/issues/4073
#    undef IPADDR_NONE

#    include "../System.h"
#    include "HttpLogClient.h"

namespace {
    const char* _state_name[4] = {
        "WRITING_HEADER",
        "LOGGING",
        "FINISHED",
    };

    const char content_length[] = "Content-Length:";

    const char header_delimiter[] = "\r\n";

    // The print completed successfully.
    const char ok_200[] = "HTTP/1.1 200 OK\r\n"
                          "Access-Control-Allow-Origin: *" "\r\n"
                          "Access-Control-Allow-Headers: *" "\r\n"
                          "Content-Type: text/plain; charset=us-ascii" "\r\n"
                          "Cache-Control: no-cache" "\r\n"
                          "Connection: keep-alive" "\r\n"
                          "X-Content-Type-Options: nosniff" "\r\n"
                          "\r\n";
}

HttpLogClient::HttpLogClient(
    const char *name, WiFiClient wifi_client) :
    Channel(name), _state(WRITING_HEADER), _wifi_client(wifi_client) {
      allChannels.registration(this);
    }

HttpLogClient::~HttpLogClient() {
  allChannels.deregistration(this);
}

void HttpLogClient::done() {
    set_state(FINISHED);
}

bool HttpLogClient::is_done() {
    return _state == FINISHED;
}

void HttpLogClient::set_state(State state) {
    if (_state != state) {
        _state = state;
    }
}

void HttpLogClient::handle() {
  if (_wifi_client.available()) {
      _wifi_client.read();
  } else if (!_wifi_client.connected()) {
      // There is nothing to read and we are not connected.
      _wifi_client.stop();
      done();
  }

  switch (_state) {
    case WRITING_HEADER: {
      _wifi_client.write(ok_200, sizeof ok_200 - 1);
      set_state(LOGGING);
      break;
    }
  }
}

int HttpLogClient::read() {
    return RETRY;
}

int HttpLogClient::peek() {
    return RETRY;
}

int HttpLogClient::available() {
    return 0;
}

size_t HttpLogClient::write(uint8_t byte) {
    if (_state != LOGGING) {
      return 1;
    }
    return _wifi_client.write(byte);
}

size_t HttpLogClient::write(const uint8_t* buffer, size_t length) {
    if (_state != LOGGING) {
      return length;
    }
    return _wifi_client.write(buffer, length);
}
