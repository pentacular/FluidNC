#include "../Config.h"

#define RETRY -1

#    include <lwip/sockets.h>

// A workaround for an issue in lwip/sockets.h
// See https://github.com/espressif/arduino-esp32/issues/4073
#    undef IPADDR_NONE

#    include "../System.h"
#    include "HttpLogClient.h"

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

HttpLogClient::HttpLogClient(const char *name, WiFiClient wifi_client) :
    Channel(name),
    _state(WRITING_HEADER), _wifi_client(wifi_client) {}

void HttpLogClient::done() {
    set_state(FINISHED);
}

bool HttpLogClient::is_done() {
    return _state == FINISHED;
}

void HttpLogClient::set_state(State state) {
    if (_state != state) {
        _state = state;
    }
}

#ifdef JSON
void HttpLogClient::ReportRealtimeStatus() {
    LogStream msg(this, "{");
    msg << "\"state\":\"" << state_name() << "\"";

    // Report position
    float* print_position = get_mpos();
    if (bits_are_true(status_mask->get(), RtStatus::Position)) {
        msg << ",\"machine_position\":[";
    } else {
        msg << ",\"work_position\":[";
        mpos_to_wpos(print_position);
    }
    msg << report_util_axis_values(print_position).c_str() << "]";

    // Returns planner and serial read buffer states.

    if (bits_are_true(status_mask->get(), RtStatus::Buffer)) {
        msg << ",\"buffer\":[" << plan_get_block_buffer_available() << "," << channel.rx_buffer_available() << "]";
    }

    if (config->_useLineNumbers) {
        // Report current line number
        plan_block_t* cur_block = plan_get_current_block();
        if (cur_block != NULL) {
            uint32_t ln = cur_block->line_number;
            if (ln > 0) {
                msg << ",\"line\":" << ln;
            }
        }
    }

    // Report realtime feed speed
    float rate = Stepper::get_realtime_rate();
    if (config->_reportInches) {
        rate /= MM_PER_INCH;
    }
    msg << ",\"speed\":[" << setprecision(0) << rate << "," << sys.spindle_speed << "]";

    if (report_pin_string.length()) {
        msg << ",\"pin\":" << report_pin_string;
    }

    if (report_wco_counter > 0) {
        report_wco_counter--;
    } else {
        switch (sys.state) {
            case State::Homing:
            case State::Cycle:
            case State::Hold:
            case State::Jog:
            case State::SafetyDoor:
                report_wco_counter = (REPORT_WCO_REFRESH_BUSY_COUNT - 1);  // Reset counter for slow refresh
            default:
                report_wco_counter = (REPORT_WCO_REFRESH_IDLE_COUNT - 1);
                break;
        }
        if (report_ovr_counter == 0) {
            report_ovr_counter = 1;  // Set override on next report.
        }
        msg << ",\"work_coordinate_offset\":[" << report_util_axis_values(get_wco()).c_str() << "]";
    }

    if (report_ovr_counter > 0) {
        report_ovr_counter--;
    } else {
        switch (sys.state) {
            case State::Homing:
            case State::Cycle:
            case State::Hold:
            case State::Jog:
            case State::SafetyDoor:
                report_ovr_counter = (REPORT_OVR_REFRESH_BUSY_COUNT - 1);  // Reset counter for slow refresh
            default:
                report_ovr_counter = (REPORT_OVR_REFRESH_IDLE_COUNT - 1);
                break;
        }

        msg << "\"speed_override\":[" << int(sys.f_override) << "," << int(sys.r_override) << "," << int(sys.spindle_speed_ovr) << "]";
        SpindleState sp_state      = spindle->get_state();
        CoolantState coolant_state = config->_coolant->get_state();
        if (sp_state != SpindleState::Disable || coolant_state.Mist || coolant_state.Flood) {
            msg << ","spindle":{";
            switch (sp_state) {
                case SpindleState::Disable:
                    msg << "\"mode\":\"disabled\"";
                    break;
                case SpindleState::Cw:
                    msg << "\"mode\":\"cw\"";
                    break;
                case SpindleState::Ccw:
                    msg << "\"mode\":\"ccw\"";
                    break;
                case SpindleState::Unknown:
                    msg << "\"mode\":\"unknown\"";
                    break;
            }

            auto coolant = coolant_state;
            if (coolant.Flood) {
                msg << ",\"flood\":true";
            }
            if (coolant.Mist) {
                msg << ",\"mist\":true";
            }
            msg << "}";
        }
    }
    if (InputFile::_progress.length()) {
        msg << ",\"input_file_process\":" + InputFile::_progress;
    }
#ifdef DEBUG_STEPPER_ISR
    msg << ",\"stepper_isr_counts\":" << Stepper::isr_count;
#endif
#ifdef DEBUG_REPORT_HEAP
    msg << ",\"heap\":" << xPortGetFreeHeapSize();
#endif
    msg << "}";
    // The destructor sends the line when msg goes out of scope
}
#endif


void HttpLogClient::handle() {
  if (_wifi_client.available()) {
      _wifi_client.read();
  } else if (!_wifi_client.connected()) {
      // There is nothing to read and we are not connected.
      _wifi_client.stop();
      done();
  }

  switch (_state) {
    case WRITING_HEADER:
      _wifi_client.write(ok_200, sizeof ok_200 - 1);
      set_state(LOGGING);
      setReportInterval(1000);
      break;
  }
}

int HttpLogClient::read() {
    return RETRY;
}

int HttpLogClient::peek() {
    return RETRY;
}

int HttpLogClient::available() {
    return 0;
}

size_t HttpLogClient::write(uint8_t byte) {
    if (_state != LOGGING) {
      return 1;
    }
    return _wifi_client.write(byte);
}

size_t HttpLogClient::write(const uint8_t* buffer, size_t length) {
    if (_state != LOGGING) {
      return length;
    }
    return _wifi_client.write(buffer, length);
}
