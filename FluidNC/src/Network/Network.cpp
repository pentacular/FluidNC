#include "../Settings.h"

#include "Network.h"

#include "../Config.h"
#include "../Logging.h"

void Network::init() {
    if (_http_batch_server) {
        _http_batch_server->init();
    }
    if (_http_log_server) {
        _http_log_server->init();
    }
    if (_http_realtime_server) {
        _http_realtime_server->init();
    }
}

void Network::handle() {
    if (_http_batch_server) {
        _http_batch_server->handle();
    }
    if (_http_log_server) {
        _http_log_server->handle();
    }
    if (_http_realtime_server) {
        _http_realtime_server->handle();
    }
}

const char* Network::name() const {
    return "network";
}

void Network::group(Configuration::HandlerBase& handler) {
    handler.section("http_batch_server", _http_batch_server);
    handler.section("http_log_server", _http_log_server);
    handler.section("http_realtime_server", _http_realtime_server);
}

void Network::afterParse() {
    // Do nothing.
}

template<>
const char* HttpServer<HttpBatchClient>::name() const {
  return "HttpBatchServer";
}

template<>
const char* HttpServer<HttpBatchClient>::client_name() const {
  return "HttpBatchClient";
}

template<>
const char* HttpServer<HttpLogClient>::name() const {
  return "HttpLogServer";
}

template<>
const char* HttpServer<HttpLogClient>::client_name() const {
  return "HttpLogClient";
}

template<>
const char* HttpServer<HttpRealtimeClient>::name() const {
  return "HttpRealtimeServer";
}

template<>
const char* HttpServer<HttpRealtimeClient>::client_name() const {
  return "HttpRealtimeClient";
}
