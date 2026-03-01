// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "clockwheel.hpp"
#include "buttonpanel.hpp"
#include "driver/gpio.h"

constexpr uint32_t kWhite = 0x222222;
constexpr uint32_t kRed = 0x220000;
constexpr uint32_t kGreen = 0x002200;
constexpr uint32_t kBlue = 0x000022;
constexpr uint32_t kYellow = 0x222200;
constexpr uint64_t kSleepDelay =
    50; // miliseconds to wait before putting motor to sleep

ClockWheel::ClockWheel(int64_t motor_update_frequency, StepperMotor &motor,
                       ButtonPanel &buttonPanelReference, int clockWheelIndex,
                       int64_t time_origin, led_strip_handle_t led_strip)
    : motor_update_frequency_(motor_update_frequency), motor_(motor),
      buttonPanelReference_(buttonPanelReference),
      clockWheelIndex_(clockWheelIndex), time_origin_(time_origin),
      led_strip_(led_strip), current_position_(0) {}

void ClockWheel::SetTimeOrigin(int64_t current_time) {
  time_origin_ = current_time;
}

void ClockWheel::Update(int64_t current_time) {

  // set elapsed time since last call
  int64_t elapsed_time = current_time - time_origin_;
  int64_t last_step_time = 0;

  int targetPosition =
      (elapsed_time * motor_.steps_per_rotation_ / motor_update_frequency_) %
      motor_.steps_per_rotation_;

  if (buttonPanelReference_.shouldPause()) {
    time_origin_ = current_time;
    current_position_ = 0;
  } else if (buttonPanelReference_.shouldReset()) {
    SetLedColor(kRed);
    motor_.ResetStep();
    time_origin_ = current_time;
    current_position_ = 0;
  } else if (buttonPanelReference_.shouldTurnCW(clockWheelIndex_)) {
    SetLedColor(kYellow);
    motor_.Step(true);
    last_step_time = current_time;
    time_origin_ = current_time;
    current_position_ = 0;
  } else if (buttonPanelReference_.shouldTurnCCW(clockWheelIndex_)) {
    SetLedColor(kGreen);
    motor_.Step(false);
    last_step_time = current_time;
    time_origin_ = current_time;
    current_position_ = 0;
  } else {
    // normal operation (turn if needed)
    SetLedColor(kWhite);
    if (current_position_ != targetPosition) {
      motor_.Step(true);
      last_step_time = current_time;
      current_position_ = (current_position_ + 1) % motor_.steps_per_rotation_;
    }
  }
  if (current_time - last_step_time > kSleepDelay) {
    // if no step has been taken for more than 1ms, put the motor to sleep
    motor_.Sleep();
  }
}

void ClockWheel::SetLedColor(uint32_t color) {
  if (led_strip_ != nullptr) {
    led_strip_set_pixel(led_strip_, 0, color >> 16 & 0xFF, color >> 8 & 0xFF,
                        color & 0xFF);
    led_strip_refresh(led_strip_);
  }
}
