#pragma once

#include "../Config.h"

#ifdef INCLUDE_HTTP_PRINT_SERVICE

#    include <WiFi.h>

#    include "../Configuration/Configurable.h"
#    include "../Serial.h"
#    include "HttpPrintClient.h"

// This provides a service for streaming gcode for printing via http post.
//
// See 'print.html' for an example of how to call the print service from a
// browser.
//
// Example yaml configuration:
//
// network:
//   HttpPrintServer:
//     port: 88

class HttpPrintServer : public Configuration::Configurable, public Channel {
private:
    enum State {
        UNSTARTED,
        IDLE,
        PRINTING,
        STOPPED,
    };

public:
    HttpPrintServer();

    // The server will now start accepting connections.
    bool begin();

    // The server will accept no new connections.
    void stop();

    // Call this periodically to allow new connections to be established.
    void handle();

    // Configuration
    const char* name() const;
    void        afterParse() override;
    void        group(Configuration::HandlerBase& handler) override;
    void        init();

    // Stream interface.
    int    read() override { return -1; }
    int    peek() override { return -1; }
    void   flush() override {};
    int    available() override { return 0; }
    size_t write(uint8_t) override { return 0; }

private:
    void setState(State state);

    enum State      _state;
    int             _port;
    WiFiServer      _server;
    HttpPrintClient _client;
};

#endif  // INCLUDE_HTTP_PRINT_SERVICE
