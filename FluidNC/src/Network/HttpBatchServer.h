#pragma once

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

class HttpPrintServer : public Configuration::Configurable {
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

private:
    void setState(State state);

    enum State      _state;
    int             _port;
    WiFiServer      _server;
    std::unique_ptr<HttpPrintClient> _client;
};
