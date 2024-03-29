#pragma once

#    include <WiFi.h>

#    include "../Configuration/Configurable.h"
#    include "../Serial.h"

template <typename HttpClient>
class HttpServer : public Configuration::Configurable {
 private:
  enum State {
    UNSTARTED,
    IDLE,
    RUNNING,
    STOPPED,
  };

  const char* _state_name[4] = {
    "UNSTARTED",
    "IDLE",
    "RUNNING",
    "STOPPED",
  };

 public:
  HttpServer() : _state(UNSTARTED), _port(0) {}

  bool begin() {
    if (_state != UNSTARTED || _port == 0) {
      return false;
    }
    _server = WiFiServer(_port);
    _server.begin();
    set_state(IDLE);
    return true;
  }

  void stop() {
    if (_state == STOPPED) {
        return;
    }
    _server.stop();
    set_state(STOPPED);
  }

  void handle() {
    switch (_state) {
      case UNSTARTED:
      case STOPPED: {
        return;
      }
      case IDLE: {
        if (_server.hasClient()) {
          _client = make_client();
          set_state(RUNNING);
          // allChannels.registration(_client.get());
        }
        return;
      }
      case RUNNING: {
        _client->handle();
        if (_client->is_done()) {
          // Remove the client from the polling cycle.
          log_debug(name() << ": deregister client");
          // allChannels.deregistration(_client.get());
          if (_client->is_aborted()) {
            log_debug(name() << ": Setting HOLD due to aborted upload");
            protocol_send_event(&feedHoldEvent);
          }
          _client.reset();
          set_state(IDLE);
        }
        return;
      }
    }
  }

  // Configuration
  const char* name() const;
  void afterParse() override {};
  void init() { begin(); }

  void group(Configuration::HandlerBase& handler) {
    handler.item("port", _port);
    handler.item("report_period_ms", _report_period_ms);
  }

  std::unique_ptr<HttpClient> make_client();

 private:
  void set_state(State state) {
    if (_state != state) {
      _state = state;
    }
  }

  enum State      _state;
  int             _port;
  uint32_t        _report_period_ms;
  WiFiServer      _server;
  std::unique_ptr<HttpClient> _client;
};
