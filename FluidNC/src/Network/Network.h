#pragma once

#include "HttpBatchClient.h"
#include "HttpLogClient.h"
#include "HttpRealtimeClient.h"
#include "HttpStatusClient.h"
#include "HttpServer.h"
#include "HttpUploadClient.h"

#include "../Config.h"
#include "../Configuration/Configurable.h"

class Network : public Configuration::Configurable {
public:
    Network() = default;

    Network(const Network&) = delete;
    Network(Network&&)      = delete;
    Network& operator=(const Network&) = delete;
    Network& operator=(Network&&) = delete;

    void init();
    void handle();

    // Configuration
    const char* name() const;
    void        afterParse() override;
    void        group(Configuration::HandlerBase& handler) override;

    // Virtual base classes require a virtual destructor.
    virtual ~Network() {}

private:
    HttpServer<HttpBatchClient>* _http_batch_server;
    HttpServer<HttpUploadClient>* _http_sd_upload_server;
    HttpServer<HttpUploadClient>* _http_localfs_upload_server;
    HttpServer<HttpLogClient>* _http_log_server;
    HttpServer<HttpStatusClient>* _http_status_server;
    HttpServer<HttpRealtimeClient>* _http_realtime_server;
};
