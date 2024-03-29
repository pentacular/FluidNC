#include "../Config.h"
#include "../Serial.h"

#    include "../Protocol.h"
#    include "../Settings.h"

#    include "HttpLogServer.h"

namespace {
    const char* _state_name[4] = {
        "UNSTARTED",
        "IDLE",
        "LOGGING",
        "STOPPED",
    };
}

HttpLogServer::HttpLogServer() : _state(UNSTARTED), _port(0) {
}

bool HttpLogServer::begin() {
    if (_state != UNSTARTED || _port == 0) {
        return false;
    }
    _server = WiFiServer(_port);
    _server.begin();
    setState(IDLE);
    return true;
}

void HttpLogServer::stop() {
    if (_state == STOPPED) {
        return;
    }
    _server.stop();
    setState(STOPPED);
}

void HttpLogServer::handle() {
    switch (_state) {
        case UNSTARTED:
        case STOPPED:
            return;
        case IDLE:
            if (_server.hasClient()) {
                _client = std::make_unique<HttpLogClient>(_server.available());
                setState(LOGGING);
                allChannels.registration(_client.get());
            }
            return;
        case LOGGING:
            _client->handle();
            if (_client->is_done()) {
                // Remove the client from the polling cycle.
                allChannels.deregistration(_client.get());
                _client.reset();
                setState(IDLE);
                log_debug("Disconnected");
            }
            return;
    }
}

void HttpLogServer::setState(State state) {
    if (_state != state) {
        _state = state;
    }
}

void HttpLogServer::init() {
  begin();
}

const char* HttpLogServer::name() const {
    return "HttpLogServer";
}

void HttpLogServer::group(Configuration::HandlerBase& handler) {
    handler.item("port", _port);
}

void HttpLogServer::afterParse() {
}
