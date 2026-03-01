// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

#include "driver/gpio.h"
#include "stepper-motor.hpp"

/**
 * Four-pin stepper motor driver using GPIO outputs.
 */
class StepperMotor4P : public StepperMotor {
public:
  static constexpr int kNumberOfPins = 4;

  /**
   * Create a four-pin stepper motor driver with pin assignments.
   */
  StepperMotor4P(int steps_per_rotation, gpio_num_t motor_pin_0,
                 gpio_num_t motor_pin_1, gpio_num_t motor_pin_2,
                 gpio_num_t motor_pin_3);

  /**
   * Advance the motor by one step.
   * @param clockwise true to step clockwise; false for counter-clockwise.
   */
  void Step(bool clockwise = true) override;
  void Sleep() override;

private:
  gpio_num_t motor_pins_[kNumberOfPins];
};
