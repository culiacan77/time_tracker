// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

#include "driver/gpio.h"
#include "led_strip.h"

// position des leds à allumer selon le pattern
namespace LightingPattern {
constexpr uint64_t KSNAKE = 0x147852369;
constexpr uint64_t KCirclePattern = 0x14789632;
constexpr uint64_t KPAUSE = 0x123789;
} // namespace LightingPattern

class LedPanel {
public:
  /**
   * @param kLedPanelNumber: number of leds in the panel
   * @param kLedLifespan time before a turned on led is turned off;
   * @param lightingPattern lighting order of the leds;
   * @param kLightingPatternSpeed speed of the lighting pattern;
   */
  LedPanel(int kLedPanelNumber, int kLedLifespan, int kLightingPattern,
           gpio_num_t switch_left, gpio_num_t switch_right,
           int updateFrequency);

  /**
   *Set the time origin to synchronize motor position.
   *@param current_time Current time in microseconds.
   */
  void SetTimeOrigin(int64_t current_time);

  void Update(int64_t current_time);

protected:
private:
  int LedTrail_; // nbr led allumées derrière la led principale
  int lightingPattern_;
  int LightingPatternSpeed_;
  int64_t time_origin_; // aussi nommé ainsi dans clockwheel problème ?
  int current_position_ = 0;
  int ledPanelIndex_ = 0;
  uint64_t currentLedPattern_;
  int currentPatternLength_;
  uint64_t currentPattern; // store current lighting pattern
  gpio_num_t switch_left_;
  gpio_num_t switch_right_;
  int updateFrequency_;
  double update_tracker_ = 0.0;

  int getPatternLength(uint64_t ledPattern);
};