#include "MachineConfig.h"

char Machine::MachineConfig::defaultConfig[] = R"config(

board: None
name: "Wall Plotter"

start:
  must_home: false

stepping:
  engine: RMT
  idle_ms: 250
  pulse_us: 4
  dir_delay_us: 0
  disable_delay_us: 0

axes:
  shared_stepper_disable_pin: gpio.12:high
  
  x:
    steps_per_mm: 32.6114649681
    max_rate_mm_per_min: 200
    acceleration_mm_per_sec2: 10
    max_travel_mm: 1000
    motor0:
      stepstick:
        step_pin: gpio.14
        direction_pin: gpio.27

  y:
    steps_per_mm: 32.6114649681
    max_rate_mm_per_min: 200
    acceleration_mm_per_sec2: 10
    max_travel_mm: 1000
    motor0:
      stepstick:
        step_pin: gpio.16
        direction_pin: gpio.17

  z:
    steps_per_mm: 1
    max_travel_mm: 100
    max_rate_mm_per_min: 1000
    acceleration_mm_per_sec2: 100
    homing:
      cycle: 1
      mpos_mm: 100
    motor0:
      rc_servo:
        pwm_hz: 50
        output_pin: gpio.26
        min_pulse_us: 1000
        max_pulse_us: 2000

kinematics:
  WallPlotter:
    left_axis: 0
    left_anchor_x: -267
    left_anchor_y: 250

    right_axis: 1
    right_anchor_x: 267
    right_anchor_y: 250

    segment_length: 10

)config";
