// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

#include "driver/gpio.h"
#include "led_strip.h"
#include "stepper-motor.hpp"

/**
 * Clock wheel display with stepper motor control and optional status LED.
 * Synchronizes motor position to elapsed time from an origin point.
 */
class ClockWheel {
   public:
    /**
     * Create a clock wheel with switch inputs and motor control.
     * @param Switch_up GPIO pin for up button.
     * @param Switch_down GPIO pin for down button.
     * @param motor_update_frequency Update frequency in microseconds.
     * @param motor Reference to stepper motor driver.
     * @param time_origin Optional initial time reference (default: 0).
     * @param led_strip Optional LED ring handle for color display (default: nullptr).
     */
    ClockWheel(gpio_num_t Switch_up,
               gpio_num_t Switch_down,
               int64_t motor_update_frequency,
               StepperMotor& motor,
               int64_t time_origin          = 0,
               led_strip_handle_t led_strip = nullptr);

    /**
     * Set the time origin to synchronize motor position.
     * @param current_time Current time in microseconds.
     */
    void SetTimeOrigin(int64_t current_time);

    /**
     * Update motor position based on elapsed time.
     * @param current_time Current time in microseconds.
     */
    void Update(int64_t current_time);

    /**
     * Set the color of the LED ring.
     * @param color RGB color value (24-bit).
     */
    void SetLedColor(uint32_t color);

   private:
    StepperMotor& motor_;  // composition, clockwheel HAS A motor
    gpio_num_t switch_up_;
    gpio_num_t switch_down_;
    int64_t motor_update_frequency_;
    int64_t time_origin_;
    led_strip_handle_t led_strip_;
    int current_position_ = 0;
};