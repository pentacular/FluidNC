#pragma once

#include <WiFi.h>
#include "../Channel.h"
#include "../FileStream.h"
#include "../FluidPath.h"
#include "../Serial.h"

// Opens a connection and then ignores a header terminated by \r\n\r\n.
// The remainder of the data is output via single character read().
// isFinished() indicates that the client is closed and exhausted.

class HttpUploadClient {
 public:
  enum State {
    READING_POST_HEADER,
    OPENING_FILE,
    READING_DATA,
    CLOSING_FILE,
    WRITING_POST_HEADER,
    FINISHING,
    FINISHED,
};

  HttpUploadClient(const char* name, WiFiClient wifi_client, const std::string& fs);
  ~HttpUploadClient();

  void abort();
  void advance();
  void done();
  bool is_aborted();
  bool is_done();

  void handle();

  int direct_read();

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

  std::string _filename;
  std::string _fs;
  FluidPath _fpath;
  std::unique_ptr<FileStream> _uploadFile;
};
