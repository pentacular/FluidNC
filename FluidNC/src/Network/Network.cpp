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
    if (_http_status_server) {
        _http_status_server->init();
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
    if (_http_status_server) {
        _http_status_server->handle();
    }
}

const char* Network::name() const {
    return "network";
}

void Network::group(Configuration::HandlerBase& handler) {
    handler.section("http_batch_server", _http_batch_server);
    handler.section("http_log_server", _http_log_server);
    handler.section("http_status_server", _http_status_server);
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
const char* HttpServer<HttpLogClient>::name() const {
  return "HttpLogServer";
}

template<>
const char* HttpServer<HttpRealtimeClient>::name() const {
  return "HttpRealtimeServer";
}

template<>
const char* HttpServer<HttpStatusClient>::name() const {
  return "HttpStatusServer";
}

template<>
std::unique_ptr<HttpBatchClient> HttpServer<HttpBatchClient>::make_client() {
  return std::make_unique<HttpBatchClient>("HttpBatchClient", _server.available());
}

template<>
std::unique_ptr<HttpLogClient> HttpServer<HttpLogClient>::make_client() {
  return std::make_unique<HttpLogClient>("HttpLogClient", _server.available());
}

template<>
std::unique_ptr<HttpRealtimeClient> HttpServer<HttpRealtimeClient>::make_client() {
  return std::make_unique<HttpRealtimeClient>("HttpRealtimeClient", _server.available());
}

template<>
std::unique_ptr<HttpStatusClient> HttpServer<HttpStatusClient>::make_client() {
  return std::make_unique<HttpStatusClient>(
      "HttpStatusClient", _server.available(), _report_period_ms);
}

