#include "../Config.h"
#include "../Serial.h"

#ifdef INCLUDE_HTTP_PRINT_SERVICE

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

HttpPrintServer::HttpPrintServer() : _state(UNSTARTED), _port(0), Channel("HttpPrintServer") {}

bool HttpPrintServer::begin() {
    if (_state != UNSTARTED || _port == 0) {
        return false;
    }
    _server = WiFiServer(_port);
    _server.begin();
    setState(IDLE);
    allChannels.registration(this);
    return true;
}

void HttpPrintServer::stop() {
    if (_state == STOPPED) {
        return;
    }
    allChannels.deregistration(this);
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
                _client = HttpPrintClient(_server.available());
                setState(PRINTING);
                allChannels.registration(&_client);
            }
            return;
        case PRINTING:
            if (_client.is_done()) {
                // Remove the client from the polling cycle.
                allChannels.deregistration(&_client);
                if (_client.is_aborted()) {
                    log_debug("HttpPrintServer: Setting HOLD due to aborted upload");
                    rtFeedHold = true;
                }
                setState(IDLE);
            }
            return;
    }
}

void HttpPrintServer::setState(State state) {
    if (_state != state) {
        log_debug("HttpPrintServer: " << _state_name[state]);
        _state = state;
    }
}

void HttpPrintServer::init() {
    log_debug("HttpPrintServer init");
    begin();
    log_debug("HttpPrintServer port=" << _port);
}

const char* HttpPrintServer::name() const {
    return "HttpPrintServer";
}

void HttpPrintServer::group(Configuration::HandlerBase& handler) {
    handler.item("port", _port);
}

void HttpPrintServer::afterParse() {
    // Do nothing.
}

#endif  // INCLUDE_HTTP_PRINT_SERVICE
