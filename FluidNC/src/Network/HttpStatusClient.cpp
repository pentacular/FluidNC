#include "../Config.h"
#include "../InputFile.h"
#include "../Machine/MachineConfig.h"
#include "../Planner.h"
#include "../SettingsDefinitions.h"
#include "../Types.h"

#define RETRY -1

#include <lwip/sockets.h>

// A workaround for an issue in lwip/sockets.h
// See https://github.com/espressif/arduino-esp32/issues/4073
#undef IPADDR_NONE

#include "../System.h"
#include "HttpStatusClient.h"

namespace {
  const char* _state_name[4] = {
    "WRITING_HEADER",
    "LOGGING",
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
                        "X-Content-Type-Options: nosniff" "\r\n"
                        "\r\n";
}

HttpStatusClient::HttpStatusClient(
    const char *name, WiFiClient wifi_client, uint32_t report_period_ms) :
    Channel(name), _state(WRITING_HEADER), _wifi_client(wifi_client),
    _report_period_ms(report_period_ms),
    _report_next_tick(usToEndTicks(_report_period_ms * 1000)),
    _last_status("")
    {}

void HttpStatusClient::done() {
    set_state(FINISHED);
}

bool HttpStatusClient::is_done() {
    return _state == FINISHED;
}

void HttpStatusClient::set_state(State state) {
    if (_state != state) {
        _state = state;
    }
}

void HttpStatusClient::report_realtime_status() {
  std::string status;
  {
    std::ostringstream msg;

    msg << "{";
    msg << "\"state\":\"" << state_name() << "\"";

    // Report position
    float* print_position = get_mpos();
    msg << ",\"machine_position\":[" << report_util_axis_values(print_position) << "]";

    mpos_to_wpos(print_position);
    msg << ",\"work_position\":[" << report_util_axis_values(print_position) << "]";

    plan_block_t* cur_block = plan_get_current_block();
    if (cur_block == NULL) {
      msg << ",\"line\":0";
    } else {
      uint32_t ln = cur_block->line_number;
      msg << ",\"line\":" << ln;
    }

    // Report realtime feed speed
    float rate = Stepper::get_realtime_rate();
    if (config->_reportInches) {
        rate /= MM_PER_INCH;
    }
    msg << ",\"rate\":" << (int)rate;
    switch (spindle->get_state()) {
      case SpindleState::Disable:
        msg << ",\"speed\":0";
        break;
      case SpindleState::Cw:
        msg << ",\"speed\":" << sys.spindle_speed;
        break;
      case SpindleState::Ccw:
        msg << ",\"speed\":-" << sys.spindle_speed;
        break;
      case SpindleState::Unknown:
        msg << ",\"speed\":null";
        break;
    }
    msg << ",\"wco\":[" << report_util_axis_values(get_wco()).c_str() << "]";
    msg << ",\"fro\":[" << int(sys.f_override) << "," << int(sys.r_override) << "," << int(sys.spindle_speed_ovr) << "]";
    msg << ",\"ram\":" << xPortGetFreeHeapSize();
    msg << "}";

    status = msg.str();
  }

  if (status != _last_status) {
    _wifi_client.write(status.c_str(), status.size());
    _last_status = std::move(status);
  }
}

void HttpStatusClient::handle() {
  if (_wifi_client.available()) {
      _wifi_client.read();
  } else if (!_wifi_client.connected()) {
      // There is nothing to read and we are not connected.
      _wifi_client.stop();
      done();
  }

  switch (_state) {
    case WRITING_HEADER: {
      _wifi_client.write(ok_200, sizeof ok_200 - 1);
      set_state(LOGGING);
      break;
    }
    case LOGGING: {
      if (getCpuTicks() - _report_next_tick > 0) {
        _report_next_tick = usToEndTicks(_report_period_ms * 1000);
        report_realtime_status();
      }
    }
  }
}

int HttpStatusClient::read() {
    return RETRY;
}

int HttpStatusClient::peek() {
    return RETRY;
}

int HttpStatusClient::available() {
    return 0;
}

size_t HttpStatusClient::write(uint8_t byte) {
    if (_state != LOGGING) {
      return 1;
    }
    return _wifi_client.write(byte);
}

size_t HttpStatusClient::write(const uint8_t* buffer, size_t length) {
    if (_state != LOGGING) {
      return length;
    }
    return _wifi_client.write(buffer, length);
}
