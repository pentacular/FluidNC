#include "../Config.h"
#include "../Serial.h"

#    include "../Protocol.h"
#    include "../Settings.h"

#    include "HttpPrintServer.h"

namespace {
    const char* _state_name[4] = {
        "UNSTARTED",
        "IDLE",
        "PRINTING",
        "STOPPED",
    };
}

HttpPrintServer::HttpPrintServer() : _state(UNSTARTED), _port(0) {
}

bool HttpPrintServer::begin() {
    if (_state != UNSTARTED || _port == 0) {
        return false;
    }
    _server = WiFiServer(_port);
    _server.begin();
    setState(IDLE);
    return true;
}

void HttpPrintServer::stop() {
    if (_state == STOPPED) {
        return;
    }
    _server.stop();
    setState(STOPPED);
}

void HttpPrintServer::handle() {
    switch (_state) {
        case UNSTARTED:
        case STOPPED:
            return;
        case IDLE:
            if (_server.hasClient()) {
                _client = std::make_unique<HttpPrintClient>(_server.available());
                setState(PRINTING);
                log_debug("HttpPrintServer: register client");
                allChannels.registration(_client.get());
                log_debug("HttpPrintServer: register client done");
            }
            return;
        case PRINTING:
            _client->handle();
            if (_client->is_done()) {
                // Remove the client from the polling cycle.
                log_debug("HttpPrintServer: deregister client");
                allChannels.deregistration(_client.get());
                if (_client->is_aborted()) {
                    log_debug("HttpPrintServer: Setting HOLD due to aborted upload");
                    protocol_send_event(&feedHoldEvent);
                }
                _client.reset();
                setState(IDLE);
            }
            return;
    }
}

void HttpPrintServer::setState(State state) {
    if (_state != state) {
        _state = state;
    }
}

void HttpPrintServer::init() {
  begin();
}

const char* HttpPrintServer::name() const {
    return "HttpPrintServer";
}

void HttpPrintServer::group(Configuration::HandlerBase& handler) {
    handler.item("port", _port);
}

void HttpPrintServer::afterParse() {
}
