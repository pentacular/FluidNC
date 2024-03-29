#include "../Config.h"
#include "../Serial.h"

#define RETRY -1

#    include <lwip/sockets.h>

// A workaround for an issue in lwip/sockets.h
// See https://github.com/espressif/arduino-esp32/issues/4073
#    undef IPADDR_NONE

#    include "../System.h"
#    include "HttpBatchClient.h"

namespace {
    const char* _state_name[] = {
        "READING_OPTIONS_HEADER",
        "WRITING_OPTIONS_HEADER",
        "READING_POST_HEADER",
        "WRITING_POST_HEADER",
        "READING_DATA",
        "READING_DELIMITER",
        "FINISHING",
        "FINISHED",
    };

    const char content_length[] = "Content-Length:";

    const char header_delimiter[] = "\r\n";

    // The print completed successfully.
    const char ok_200[] = "HTTP/1.1 200 OK\r\n"
                          "Access-Control-Allow-Origin: *" "\r\n"
                          "Access-Control-Allow-Headers: *" "\r\n"
                          "Content-Type: text/plain; charset=us-ascii" "\r\n"
                          "Cache-Control: no-cache" "\r\n"
                          "Connection: keep-alive" "\r\n"
                          "Content-Length: 0" "\r\n"
                          "X-Content-Type-Options: nosniff" "\r\n"
                          "\r\n";
}

HttpBatchClient::HttpBatchClient(const char *name, WiFiClient wifi_client) :
    Channel(name),
    _state(READING_OPTIONS_HEADER), _wifi_client(wifi_client), _content_read(0),
    _content_size(0), _data_read(0), _data_size(0), _aborted(false), _need_ack(false) {
  allChannels.registration(this);
}

HttpBatchClient::~HttpBatchClient() {
  allChannels.deregistration(this);
}

bool HttpBatchClient::is_done() {
    return _state == FINISHED;
}

bool HttpBatchClient::is_aborted() {
    return _aborted;
}

void HttpBatchClient::clear_ack() {
  _need_ack = true;
}

void HttpBatchClient::set_ack() {
  _need_ack = false;
}

bool HttpBatchClient::need_ack() {
  return _need_ack;
}

void HttpBatchClient::set_state(State state) {
    if (_state != state) {
        // Show the state changes so we can see what's happening via other
        // clients.
        _state = state;
    }
}

Channel* HttpBatchClient::pollLine(char* line) {
    // If line == nullptr this is a reques for realtime data,
    // which we do not supply.
    if (!line || need_ack()) {
        return nullptr;
    }
    Channel* channel = Channel::pollLine(line);
    if (!channel) {
      return nullptr;
    }
    // Block further reads until we ack this line.
    clear_ack();
    return channel;
}

void HttpBatchClient::ack(Error status) {
    if (status != Error::Ok) {
        log_debug("Ack error: aborting");
        abort();
    }
    set_ack();
}

void HttpBatchClient::abort() {
  _aborted = true;
  done();
}

void HttpBatchClient::done() {
  set_state(FINISHING);
}

void HttpBatchClient::advance() {
  _data_read++;
  _content_read++;
}

void HttpBatchClient::handle() {
    switch(_state) {
        case WRITING_OPTIONS_HEADER: {
          // Send response headers (assuming this will fit in buffers).
          if (_wifi_client.write(ok_200, sizeof ok_200 - 1) != sizeof ok_200 -1) {
            log_debug("HttpBatchClient options header truncated");
          }
          set_state(READING_POST_HEADER);
          return;
        }
        case WRITING_POST_HEADER: {
          // Send response headers (assuming this will fit in buffers).
          if (_wifi_client.write(ok_200, sizeof ok_200 - 1) != sizeof ok_200 -1) {
            log_debug("HttpBatchClient post header truncated");
          }
          set_state(READING_DATA);
          return;
        }
        case FINISHING: {
            if (need_ack()) {
              return;
            }
            shutdown(_wifi_client.fd(), SHUT_RDWR);
            log_debug("HttpBatchClient done");
            set_state(FINISHED);
            return;
        }
        case READING_DATA:
        case FINISHED:
        case READING_OPTIONS_HEADER:
        case READING_POST_HEADER:
        case READING_DELIMITER:
            return;
    }
}

int HttpBatchClient::direct_read() {
    if (is_data_exhausted()) {
        // The header line was too long, throw away the start.
        reset_data();
    }
    int code = _wifi_client.read();
    if (code == RETRY) {
        return code;
    }
    _data[_data_size++] = code;
    return code;
}

int HttpBatchClient::read() {
    switch (_state) {
        case FINISHED:
            return RETRY;
        case FINISHING:
            return RETRY;
        case READING_OPTIONS_HEADER: {
            if (!_wifi_client.connected()) {
              // Breaking off while reading the header is safe.
              done();
              return RETRY;
            }
            int code = direct_read();
            if (code == RETRY) {
                return code;
            }
            if (code == '\n') {
                // log_debug("_data_read=" << (int)_data_read << " _data_size=" << (int)_data_size);
                // log_debug("Header line: " << std::string(_data + _data_read, _data_size - _data_read));
                if (strncmp(_data, header_delimiter, sizeof header_delimiter - 1) == 0) {
                    // log_debug("Options header delimiter");
                    set_state(WRITING_OPTIONS_HEADER);
                }
                reset_data();
            }
            return RETRY;
        }
        case READING_POST_HEADER: {
            if (!_wifi_client.connected()) {
              // Breaking off while reading the header is safe.
              done();
              return RETRY;
            }
            int code = direct_read();
            if (code == RETRY) {
                return code;
            }
            if (code == '\n') {
                // We have a complete line.
                if (strncmp(_data, content_length, sizeof content_length - 1) == 0) {
                    // Content-Length: 1234
                    _content_size = atol(_data + sizeof content_length);
                    // log_debug("content_length=" << (int)_content_size);
                } else if (strncmp(_data, header_delimiter, sizeof header_delimiter - 1) == 0) {
                    // log_debug("Post header delimiter");
                    set_state(WRITING_POST_HEADER);
                }
                reset_data();
            }
            return RETRY;
        }
        case READING_DATA: {
            if (need_ack()) {
                return RETRY;
            }
            if (is_content_exhausted()) {
                // No more to read.
                if (!need_ack()) {
                  // Wait for the next chunk.
                  set_state(READING_OPTIONS_HEADER);
                }
                return RETRY;
            }
            int code = peek();
            if (code == RETRY) {
              return code;
            }
            // log_debug("read=" << (char)code);
            advance();
            return code;
        }
        case READING_DELIMITER: {
            if (!_wifi_client.connected()) {
              // Breaking off while reading the header is safe.
              done();
              return RETRY;
            }

            int code = direct_read();
            if (code == RETRY) {
                return code;
            }

            if (code == '\n') {
                if (strncmp(_data, header_delimiter, sizeof header_delimiter - 1) == 0) {
                    // log_debug("Data delimiter");
                    set_state(READING_POST_HEADER);
                }
                reset_data();
            }
            return RETRY;
        }
    }
    // Unreachable
    return RETRY;
}

int HttpBatchClient::peek() {
    if (_state != READING_DATA) {
        return RETRY;
    }
    if (is_data_exhausted()) {
        size_t available = _wifi_client.available();
        if (available == 0) {
            if (!_wifi_client.connected()) {
                log_debug("Disconnected");
                // There is nothing to read and we are not connected.
                _wifi_client.stop();
                abort();
            }
            return RETRY;
        }
        _data_read = 0;
        if (available < sizeof _data) {
          _data_size = _wifi_client.readBytes(_data, available);
        } else {
          _data_size = _wifi_client.readBytes(_data, sizeof _data);
        }
    }
    return _data[_data_read];
}

void HttpBatchClient::flush() {
    if (_state != READING_DATA) {
        return;
    }
    _wifi_client.flush();
}

int HttpBatchClient::available() {
    if (_state != READING_DATA) {
        return 0;
    }
    return _wifi_client.available();
}

size_t HttpBatchClient::write(uint8_t) {
    return 0;
}
