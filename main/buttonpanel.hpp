// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

#include "driver/gpio.h"
#include <vector>

class ButtonPanel {
public:
  // constructeur
  ButtonPanel(gpio_num_t minuteSwitchUp, gpio_num_t minuteSwitchDown,
              gpio_num_t hourSwitchUp, gpio_num_t hourSwitchDown,
              gpio_num_t daySwitchUp, gpio_num_t daySwitchDown,
              gpio_num_t ledPanelSwitchUp, gpio_num_t ledPanelSwitchDown);
  /**
   *Set the time origin to synchronize motor position.
   *@param current_time Current time in microseconds.
   */

  // methodes
  bool shouldReset();
  bool shouldTurnCW(int clockWheelIndex);
  bool shouldTurnCCW(int clockWheelIndex);
  bool shouldChangeColor();
  bool shouldPause();

protected:
private:
  gpio_num_t minuteSwitchUp_;
  gpio_num_t minuteSwitchDown_;
  gpio_num_t hourSwitchUp_;
  gpio_num_t hourSwitchDown_;
  gpio_num_t daySwitchUp_;
  gpio_num_t daySwitchDown_;
  gpio_num_t ledPanelSwitchUp_;
  gpio_num_t ledPanelSwitchDown_;
  // vecteur pour stocker les pins des switchs
  std::vector<gpio_num_t> switchPins_;
};
