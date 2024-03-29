#pragma once

#    include <WiFi.h>
#    include "../Channel.h"
#    include "../Serial.h"

// Opens a connection and then ignores a header terminated by \r\n\r\n.
// The remainder of the data is output via single character read().
// isFinished() indicates that the client is closed and exhausted.

class HttpBatchClient : public Channel {
public:
    enum State {
        READING_OPTIONS_HEADER,
        WRITING_OPTIONS_HEADER,
        READING_POST_HEADER,
        WRITING_POST_HEADER,
        READING_DATA,
        READING_DELIMITER,
        FINISHING,
        FINISHED,
    };

    HttpBatchClient(const char* name, WiFiClient wifi_client);

    void abort();
    void advance();
    void clear_ack();
    void done();
    void set_ack();
    bool is_aborted();
    bool is_done();
    bool need_ack();

    void handle() override;

    int direct_read();

    // Stream interface.
    int    read() override;
    int    peek() override;
    void   flush() override;
    int    available() override;
    size_t write(uint8_t) override;

    void ack(Error status) override;
    Channel* pollLine(char* line) override;

protected:
    void set_state(State state);

    inline size_t is_content_exhausted() const { return _content_size == _content_read; }
    inline size_t is_data_exhausted() const { return _data_size == _data_read; }
    inline void   reset_data() {
        _data_read = 0;
        _data_size = 0;
        _content_read = 0;
    }

    enum State _state;
    WiFiClient _wifi_client;

    long   _content_read;
    long   _content_size;
    char   _data[128];
    size_t _data_read;
    size_t _data_size;
    bool   _aborted;
    bool   _need_ack;
};
