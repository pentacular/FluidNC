#include "../Config.h"
#include "../Serial.h"

#define RETRY -1

#include <lwip/sockets.h>

// A workaround for an issue in lwip/sockets.h
// See https://github.com/espressif/arduino-esp32/issues/4073
#undef IPADDR_NONE

#include "../System.h"
#include "HttpUploadClient.h"

namespace {
    const char* _state_name[] = {
        "READING_POST_HEADER",
        "OPENING_FILE",
        "READING_DATA",
        "CLOSING_FILE",
        "WRITING_POST_HEADER",
        "FINISHING",
        "FINISHED",
    };

    const char content_length[] = "Content-Length:";

    const char header_delimiter[] = "\r\n";

    // The print completed successfully.
    const char ok_200[] = "HTTP/1.1 200 OK\r\n"
                          "Cache-Control: no-cache" "\r\n"
                          "Connection: Keep-Alive" "\r\n"
                          "Keep-Alive: timeout=1000, max=1000" "\r\n"
                          "Content-Length: 0" "\r\n"
                          "X-Content-Type-Options: nosniff" "\r\n"
                          "\r\n";
}

HttpUploadClient::HttpUploadClient(const char *name, WiFiClient wifi_client) :
    Channel(name),
    _state(READING_POST_HEADER), _wifi_client(wifi_client), _content_read(0),
    _content_size(0), _data_read(0), _data_size(0), _aborted(false), _need_ack(false) {
  allChannels.registration(this);
}

HttpUploadClient::~HttpUploadClient() {
  allChannels.deregistration(this);
}

bool HttpUploadClient::is_done() {
    return _state == FINISHED;
}

bool HttpUploadClient::is_aborted() {
    return _aborted;
}

void HttpUploadClient::set_state(State state) {
    if (_state != state) {
        // Show the state changes so we can see what's happening via other
        // clients.
        // log_debug("HttpUploadClient/state: " << _state_name[state]);
        _state = state;
    }
}

Channel* HttpUploadClient::pollLine(char* line) {
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

void HttpUploadClient::ack(Error status) {
    if (status != Error::Ok) {
        log_debug("Ack error: aborting");
        abort();
    }
    set_ack();
}

void HttpUploadClient::abort() {
  _aborted = true;
  done();
}

void HttpUploadClient::done() {
  set_state(FINISHING);
}

void HttpUploadClient::advance() {
  _data_read++;
  _content_read++;
}

void HttpUploadClient::handle() {
  switch(_state) {
    case READING_POST_HEADER: {
      do {
        if (!_wifi_client.connected() && !_wifi_client.available()) {
          // Breaking off while reading the header is safe.
          done();
          return;
        }
        int code = direct_read();
        if (code == RETRY) {
            continue;
        }
        if (code == '\n') {
          // We have a complete line.
          if (strncmp(_data, content_length, sizeof content_length - 1) == 0) {
            // Content-Length: 1234
            _content_size = atol(_data + sizeof content_length);
          } else if (strncmp(_data, header_delimiter, sizeof header_delimiter - 1) == 0) {
            set_state(READING_DATA);
          }
          reset_data();
        }
      } while (_wifi_client.available() > 0);
      return;
    }
    case OPENING_FILE: {
      std::error_code ec;
      _fpath = FluidPath( _filename, _fs, ec);
      if (ec) {
        log_debug("HttpUploadClient: Cannot create path");
        abort();
        return;
      }

      auto space = stdfs::space(fpath);
      if (filesize && filesize > space.available) {
        // If the file already exists, maybe there will be enough space
        // when we replace it.
        auto existing_size = stdfs::file_size(fpath, ec);
        if (ec || (filesize > (space.available + existing_size))) {
          log_debug("HttpUploadClient: Insufficient space");
          abort();
          return;
        }
      }

      try {
        _uploadFile = std::make_unique<FileStream>(fpath, "w");
      } catch (const Error err) {
        _uploadFile.reset();
        abort();
        return;
      }
    }
    case READING_DATA: {
      // Should support multipart.
      for (;;) {
        size_t available = _wifi_client.available();
        if (available == 0) {
          return;
        }
        if (available > sizeof _data) {
          available = sizeof _data;
        }
        _data_size = _wifi_client.readBytes(_data, available);
        if (_uploadFile->write(_data, _data_size) != _data_size) {
          log_debug("Partial file write");
          abort();
        }
        _content_read += _data_size;
        if (is_content_exhausted) {
          log_debug("HttpUploadClient: upload finished");
          set_state(CLOSING_FILE);
          return;
        }
      }
    }
    case CLOSING_FILE: {
      set_state(WRITING_POST_HEADER);
      return;
    }
    case WRITING_POST_HEADER: {
      // Send response headers (assuming this will fit in buffers).
      if (_wifi_client.write(ok_200, (sizeof ok_200) - 1) != (sizeof ok_200) - 1) {
        log_debug("HttpUploadClient post header truncated");
      }
      set_state(FINISHING);
      return;
    }
    case FINISHING: {
      shutdown(_wifi_client.fd(), SHUT_RDWR);
      set_state(FINISHED);
      return;
    }
    case FINISHED: {
      return;
    }
  }
}

int HttpUploadClient::direct_read() {
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

#if 0
int HttpUploadClient::peek() {
    if (_state != READING_DATA) {
        return RETRY;
    }
    if (is_data_exhausted()) {
        size_t available = _wifi_client.available();
        if (available == 0) {
            if (!_wifi_client.connected()) {
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

void HttpUploadClient::flush() {
    if (_state != READING_DATA) {
        return;
    }
    _wifi_client.flush();
}

int HttpUploadClient::available() {
    if (_state != READING_DATA) {
        return 0;
    }
    return _wifi_client.available();
}

size_t HttpUploadClient::write(uint8_t) {
    return 0;
}
#endif
