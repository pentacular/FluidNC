#pragma once

#include "../Channel.h"

#    include <WiFi.h>

#    include "../Serial.h"

// Opens a connection and then ignores a header terminated by \r\n\r\n.
// The remainder of the data is output via single character read().
// isFinished() indicates that the client is closed and exhausted.

class HttpLogClient : public Channel {
    enum State {
        WRITING_HEADER,
        LOGGING,
        FINISHED,
    };

public:
    HttpLogClient(const char *name, WiFiClient wifi_client);

    void done();
    bool is_aborted() { return false; }
    bool is_done();

    void handle() override;

    // Stream interface.
    int    read() override;
    int    peek() override;
    // void   flush() override;
    int    available() override;
    size_t write(uint8_t) override;
    size_t write(const uint8_t* buffer, size_t length) override;

private:
    void set_state(State state);

    enum State _state;
    WiFiClient _wifi_client;
};
