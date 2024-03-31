#pragma once

#include "../Channel.h"

#    include <WiFi.h>

#    include "../Serial.h"

// Opens a connection and then ignores a header terminated by \r\n\r\n.
// The remainder of the data is output via single character read().
// isFinished() indicates that the client is closed and exhausted.

class HttpStatusClient : public Channel {
    enum State {
        WRITING_HEADER,
        LOGGING,
        FINISHED,
    };

public:
    HttpStatusClient(const char *name, WiFiClient wifi_client, uint32_t report_period_ms);

    void done();
    bool is_aborted() { return false; }
    bool is_done();

    void handle() override;

    void report_realtime_status();

    // Stream interface.
    int    read() override;
    int    peek() override;
    // void   flush() override;
    int    available() override;
    size_t write(uint8_t) override;
    size_t write(const uint8_t* buffer, size_t length) override;

private:
    void set_state(State state);
    int32_t _report_period_ms;
    int32_t _report_next_tick;
    std::string _last_status;
    enum State _state;
    WiFiClient _wifi_client;
};
