#pragma once

#    include <WiFi.h>
#    include "../Channel.h"
#    include "../Serial.h"

#include "HttpBatchClient.h"

// Opens a connection and then ignores a header terminated by \r\n\r\n.
// The remainder of the data is output via single character read().
// isFinished() indicates that the client is closed and exhausted.

class HttpRealtimeClient : public HttpBatchClient {
public:
    HttpRealtimeClient(const char* name, WiFiClient wifi_client);
    virtual ~HttpRealtimeClient();

    Channel* pollLine(char* line) override;
    bool need_ack() override { return false; }
};
