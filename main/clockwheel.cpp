// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "clockwheel.hpp"

#include "driver/gpio.h"

constexpr uint32_t kWhite = 0x222222;
constexpr uint32_t kRed = 0x220000;
constexpr uint32_t kGreen = 0x002200;
constexpr uint32_t kBlue = 0x000022;
constexpr uint32_t kYellow = 0x222200;

ClockWheel::ClockWheel(gpio_num_t Switch_up, gpio_num_t Switch_down,
                       int64_t motor_update_frequency, StepperMotor &motor,
                       int64_t time_origin, led_strip_handle_t led_strip)
    : motor_(motor), switch_up_(Switch_up), switch_down_(Switch_down),
      motor_update_frequency_(motor_update_frequency),
      time_origin_(time_origin), led_strip_(led_strip), current_position_(0) {
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << switch_up_) | (1ULL << switch_down_);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void ClockWheel::SetTimeOrigin(int64_t current_time) {
  time_origin_ = current_time;
}

void ClockWheel::Update(int64_t current_time, bool should_reset) {

  // read the switch states
  int switch_up_state = gpio_get_level(switch_up_);
  int switch_down_state = gpio_get_level(switch_down_);

  // mode : 0 = minute hour day switch up (reset)
  //        1 = down switch pressed (clockwise)
  //        2 = up switch pressed (counterclockwise)
  //        3 = no switch pressed
  int mode = switch_up_state << 1 | switch_down_state;

  // override le mode si bool reset true
  if (should_reset) {
    mode = 0;
  };

  int64_t elapsed_time = current_time - time_origin_;

  int targetPosition =
      (elapsed_time * motor_.steps_per_rotation_ / motor_update_frequency_) %
      motor_.steps_per_rotation_;

  switch (mode) {
    // up switch pressed: reset
  case 0: {
    SetLedColor(kRed);
    motor_.ResetStep();
    time_origin_ = current_time;
    current_position_ = 0;
    break;
  }
  // down switch pressed (clockwise)
  case 1: {
    SetLedColor(kYellow);
    motor_.Step(true);
    time_origin_ = current_time;
    current_position_ = 0;
    break;
  }
  // up switch pressed (counterclockwise)
  case 2: {
    SetLedColor(kBlue);
    motor_.Step(false);
    time_origin_ = current_time;
    current_position_ = 0;
    break;
  }
  // normal operation (turn if needed)
  case 3: {
    SetLedColor(kWhite);
    if (current_position_ != targetPosition) {
      motor_.Step(true);
      current_position_ = (current_position_ + 1) % motor_.steps_per_rotation_;
    }
    break;
  }
  }
}

void ClockWheel::SetLedColor(uint32_t color) {
  if (led_strip_ != nullptr) {
    led_strip_set_pixel(led_strip_, 0, color >> 16 & 0xFF, color >> 8 & 0xFF,
                        color & 0xFF);
    led_strip_refresh(led_strip_);
  }
}