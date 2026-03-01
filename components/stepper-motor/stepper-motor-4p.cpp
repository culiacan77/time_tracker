// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "stepper-motor-4p.hpp"

#include <inttypes.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *kTag = "StepperMotor4P";
static constexpr uint8_t kSequenceArray[] = {0b1010, 0b0110, 0b0101, 0b1001};
// Calculate the length of the sequence array. la division permet d'avoir le nbr
// d'élément et non pas le nbr de bits
static constexpr size_t kSequenceLen =
    sizeof(kSequenceArray) / sizeof(kSequenceArray[0]);

StepperMotor4P::StepperMotor4P(int steps_per_rotation, gpio_num_t motor_pin_0,
                               gpio_num_t motor_pin_1, gpio_num_t motor_pin_2,
                               gpio_num_t motor_pin_3)
    : StepperMotor(steps_per_rotation) {
  motor_pins_[0] = motor_pin_0;
  motor_pins_[1] = motor_pin_1;
  motor_pins_[2] = motor_pin_2;
  motor_pins_[3] = motor_pin_3;

  // Configure GPIO pins as outputs
  uint64_t pin_bit_mask = 0;
  for (int i = 0; i < kNumberOfPins; ++i) {
    ESP_ERROR_CHECK(gpio_set_level(motor_pins_[i], 0));
    pin_bit_mask |= (1ULL << motor_pins_[i]);
  };
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = pin_bit_mask;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void StepperMotor4P::Step(bool clockwise) {
  StepperMotor::Step(clockwise);

  this->Sleep(); // Ensure all pins are off before setting the new step

  // Set pins according to the current step in the sequence
  uint8_t current_sequence = kSequenceArray[current_step_ % kSequenceLen];
  for (int i = 0; i < StepperMotor4P::kNumberOfPins; i++) {
    if (current_sequence & (1 << i)) {
      ESP_ERROR_CHECK(gpio_set_level(motor_pins_[i], 1));
    }
  }
  ESP_LOGD(kTag, "Current step: %d", current_step_);
}

void StepperMotor4P::Sleep() {
  // Switch off all pins
  for (int i = 0; i < StepperMotor4P::kNumberOfPins; i++) {
    ESP_ERROR_CHECK(gpio_set_level(motor_pins_[i], 0));
  }
}