// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

#include "buttonpanel.hpp"
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
   * @param motor_update_frequency Update frequency in microseconds.
   * @param motor Reference to stepper motor driver.
   * @param buttonPanelReference Reference to button panel (for switch buton
   * state).
   * @param clockWheelIndex Index of the clock wheel (minute=0, hour=1, day=2).
   * @param time_origin Optional initial time reference (default: 0).
   * @param led_strip Optional LED ring handle for color display (default:
   * nullptr).
   */
  ClockWheel(int64_t motor_update_frequency, StepperMotor &motor,
             ButtonPanel &buttonPanelReference, int clockWheelIndex,
             int64_t time_origin = 0, led_strip_handle_t led_strip = nullptr);

  /**
   * Set the time origin to synchronize motor position.
   * @param current_time Current time in microseconds.
   */
  void SetTimeOrigin(int64_t current_time);

  /**
   * Update motor position based on elapsed time.
   * @param current_time Current time in microseconds.
   * @param should_reset Boolean flag to indicate if motor should be reset.
   */
  void Update(int64_t current_time);

  /**
   * Set the color of the LED ring.
   * @param color RGB color value (24-bit).
   */
  void SetLedColor(uint32_t color);

private:
  // l'ordre dois correspondre à celui des parametres du constructeur
  int64_t motor_update_frequency_;
  StepperMotor &motor_;               // composition, clockwheel HAS A motor
  ButtonPanel &buttonPanelReference_; // référence vers la classe buttonpanel
  int clockWheelIndex_;
  int64_t time_origin_;
  led_strip_handle_t led_strip_;

  int current_position_ = 0;
};