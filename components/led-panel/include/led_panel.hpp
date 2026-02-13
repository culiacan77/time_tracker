// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

#include "driver/gpio.h"
#include "led_strip.h"
#include <vector>

// position des leds à allumer selon le pattern
namespace LightingPattern {
constexpr uint64_t KSNAKE = 0x147852369;
constexpr uint64_t KCirclePattern = 0x14789632;
constexpr uint64_t KPAUSE = 0x123789;
constexpr uint64_t KFULLLIT = 0x123456789;
} // namespace LightingPattern

class LedPanel {
public:
  LedPanel(gpio_num_t Switch_up, gpio_num_t Switch_down, gpio_num_t gpioPin,
           led_strip_rmt_config_t *rmt_config);

  /**
   *Set the time origin to synchronize motor position.
   *@param current_time Current time in microseconds.
   */

  void litLedPanel();

  void
  setAnimationPattern(const std::vector<std::vector<int>> &currentLightPattern);
  void setTrailLength(int trailLength);
  void updateMatrix(bool shouldDimLight_);
  void clearLedPanelMatrix();
  void shiftLedColor();

protected:
private:
  // matrice représentant l'indice de luminosité du led panel
  std::vector<std::vector<int>> ledPanelMatrix_{
      {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  std::vector<std::vector<int>> LightingPattern_;
  std::vector<int> BrightnessLUT_;
  led_strip_handle_t led_strip_handle_;
  gpio_num_t switch_up_;
  gpio_num_t switch_down_;

  bool shouldDimLight_ =
      true; // variable pour indiquer si les LEDs doivent être atténuées
  int LightingPatternIndex_ = 0;
  int LightingPatternSize_;
  int Hue_ = 0; // variable pour stocker la teinte actuelle du panneau
};

// selon l'état des boutons, --SetAnimationPattern et --setBrightnessLUT
// --setLedPanelMatrix vont déterminé quel vecteur (patern), et l'échelle de
// brighness de la trail.

// dimLedPanel Update matrix, litLedPanel vont gérer
//  l'animation dans la loop principale.