// Copyright (c) 2020 -	Bart Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

#pragma once

#include "Servo.h"
#include "RcServoSettings.h"

namespace MotorDrivers {
    class Solenoid : public Servo {
    protected:
        void config_message() override;

        void set_location();

        uint8_t calc_pwm_precision(uint32_t freq);

        Pin      _output_pin;
        uint32_t _pwm_freq = 5000;  // Hz
        uint8_t  _pwm_chan_num;
        uint32_t _current_pwm_duty;
        float    _peak_duty = 100.0;  // percent
        uint32_t _peak_ms   = 250;
        float    _hold_duty = 20.0;  // percent
        uint32_t   _hold_pulse_cnt;
        uint32_t   _peak_pulse_cnt;
        uint8_t    _pwm_precision;
        float      _last_mpos = 1000000.0;
        uint64_t _peak_end_time_ticks = 0;  // when will the peak pull phase end

        bool _disabled;

        int _axis_index = -1;

        bool _has_errors = false;

    public:
        Solenoid() = default;

        // Overrides for inherited methods
        void init() override;
        void read_settings() override;
        bool set_homing_mode(bool isHoming) override;
        void set_disable(bool disable) override;
        void update() override;

        void _write_pwm(uint32_t duty);

        // Configuration handlers:
        void group(Configuration::HandlerBase& handler) override {
            handler.item("output_pin", _output_pin);
            handler.item("pwm_freq", _pwm_freq);
            handler.item("peak_duty", _peak_duty);
            handler.item("peak_ms", _peak_ms);
            handler.item("hold_duty", _hold_duty);
        }

        // Name of the configurable. Must match the name registered in the cpp file.
        const char* name() const override { return "solenoid"; }
    };
}
