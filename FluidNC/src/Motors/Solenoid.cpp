// Copyright (c) 2021 -	Bart Dring
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.

/*
    This lets an solenoid be used like any other motor. 

    The solenoid will be in one position above machine 0 and the other below machine 0

    It will do an initial pull power, hold for a while, then reduce to a hold power
    It will disable like all other axes.



*/

#include "Solenoid.h"

#include "../Machine/MachineConfig.h"
#include "../System.h"  // mpos_to_steps() etc
#include "../Pins/LedcPin.h"
#include "../Pin.h"
#include "../Limits.h"  // limitsMaxPosition
#include "../NutsBolts.h"
#include <math.h>

namespace MotorDrivers {

    void Solenoid::init() {
        if (_output_pin.undefined()) {
            log_warn("    Solenoid disabled: No output pin");
            _has_errors = true;
            return;  // We cannot continue without the output pin
        }

        _axis_index = axis_index();

        read_settings();
        config_message();

        _pwm_chan_num     = ledcInit(_output_pin, -1, double(_pwm_freq), _pwm_precision);  // Allocate a channel
        _current_pwm_duty = 0;

        _disabled = true;

        startUpdateTask(_timer_ms);
    }

    /*
		Calculate the highest precision of a PWM based on the frequency in bits

		80,000,000 / freq = period
		determine the highest precision where (1 << precision) < period
	*/
    uint8_t Solenoid::calc_pwm_precision(uint32_t freq) {
        uint8_t precision = 0;
        if (freq == 0) {
            return precision;
        }

        // increase the precision (bits) until it exceeds allow by frequency the max or is 16
        while ((1u << precision) < uint32_t(80000000 / freq) && precision <= 16) {
            precision++;
        }

        return precision - 1;
    }

    void Solenoid::config_message() {
        log_info("    Solenoid " << _output_pin.name() << " peak:" << _peak_duty << "%/" << _peak_ms << "ms hold:" << _hold_duty
                                 << "% freq:" << _pwm_freq);
        log_info("    Precision:" << _pwm_precision << " Peak cnt:" << _peak_pulse_cnt << " hold cnt:" << _hold_pulse_cnt);
    }

    void Solenoid::_write_pwm(uint32_t duty) {
        // to prevent excessive calls to ledcWrite, make sure duty has changed
        if (duty == _current_pwm_duty) {
            return;
        }

        _current_pwm_duty = duty;
        ledcSetDuty(_pwm_chan_num, duty);
    }

    // sets the PWM to zero. This allows most servos to be manually moved
    void IRAM_ATTR Solenoid::set_disable(bool disable) {
        if (_has_errors)
            return;

        _disabled = disable;

        if (_disabled) {
            _write_pwm(0);
        }
    }

    // Homing justs sets the new system position and the servo will move there
    bool Solenoid::set_homing_mode(bool isHoming) {
        if (_has_errors)
            return false;

        if (isHoming) {
            auto axis                = config->_axes->_axis[_axis_index];
            motor_steps[_axis_index] = mpos_to_steps(axis->_homing->_mpos, _axis_index);
            _disabled                = false;
            set_location();  // force the PWM to update now
        }
        return false;  // Cannot be homed in the conventional way
    }

    void Solenoid::update() { set_location(); }

    void Solenoid::set_location() {
        uint32_t solenoid_pulse_len = 0;  //

        if (_disabled || _has_errors) {
            return;
        }

        float mpos = steps_to_mpos(motor_steps[_axis_index], _axis_index);  // get the axis machine position in mm

                read_settings();

        if (mpos > 0.0) {
            if (_last_mpos > 0.0) {  // we were up
                if (esp_timer_get_time() > _peak_end_time_ticks) {
                    // use hold level
                    solenoid_pulse_len = _hold_pulse_cnt;
                } else {
                    solenoid_pulse_len = _peak_pulse_cnt;
                }
            } else {
                solenoid_pulse_len   = _peak_pulse_cnt;
                _peak_end_time_ticks = esp_timer_get_time() + (1000 * _peak_ms);
            }
        } else {
            solenoid_pulse_len = 0;
        }

        _last_mpos = mpos;

        _write_pwm(solenoid_pulse_len);
    }

    void Solenoid::read_settings() {
        uint32_t max_cnt = pow(2, _pwm_precision);

        _pwm_precision  = calc_pwm_precision(_pwm_freq);
        _peak_pulse_cnt = float(max_cnt) * (_peak_duty / 100.0);
        _hold_pulse_cnt = float(max_cnt) * (_hold_duty / 100.0);
    }

    // Configuration registration
    namespace {
        MotorFactory::InstanceBuilder<Solenoid> registration("solenoid");
    }
}
