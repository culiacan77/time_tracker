// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "buttonpanel.hpp"

#include "driver/gpio.h"

/// constructeur
ButtonPanel::ButtonPanel(gpio_num_t minuteSwitchUp, gpio_num_t minuteSwitchDown,
                         gpio_num_t hourSwitchUp, gpio_num_t hourSwitchDown,
                         gpio_num_t daySwitchUp, gpio_num_t daySwitchDown,
                         gpio_num_t ledPanelSwitchUp,
                         gpio_num_t ledPanelSwitchDown)
    : minuteSwitchUp_(minuteSwitchUp), minuteSwitchDown_(minuteSwitchDown),
      hourSwitchUp_(hourSwitchUp), hourSwitchDown_(hourSwitchDown),
      daySwitchUp_(daySwitchUp), daySwitchDown_(daySwitchDown),
      ledPanelSwitchUp_(ledPanelSwitchUp),
      ledPanelSwitchDown_(ledPanelSwitchDown) {
  // On remplit le vecteur switchPins_ lors de la construction de l'objet
  switchPins_ = {minuteSwitchUp_,   minuteSwitchDown_,  hourSwitchUp_,
                 hourSwitchDown_,   daySwitchUp_,       daySwitchDown_,
                 ledPanelSwitchUp_, ledPanelSwitchDown_};

  // paramétrage des pins en tant qu'input avec pullup
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  // Configure tous les pins avec une boucle
  for (auto pin : switchPins_) {
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
  }
}

// avec le mode pull up, le switch est à 1 quand il n'est pas préssé. On veux
// que shouldReset soit true lorsque les 3 switch sont à 0.
bool ButtonPanel::shouldReset() {
  for (int i = 0; i < 3; i++) {
    if (gpio_get_level(switchPins_[i * 2]) == 1) {
      return false; // si un switch up n'est pas pressé, ne pas reset
    }
  }
  return true;
}

bool ButtonPanel::shouldTurnCCW(int clockWheelIndex) {
  return gpio_get_level(switchPins_[clockWheelIndex * 2]) == 0; // switch up
}

bool ButtonPanel::shouldTurnCW(int clockWheelIndex) {
  return gpio_get_level(switchPins_[clockWheelIndex * 2 + 1]) ==
         0; // switch down
}

bool ButtonPanel::shouldChangeColor() {
  return gpio_get_level(switchPins_[6]) == 0; // led panel switch up
}

bool ButtonPanel::shouldPause() {
  return gpio_get_level(switchPins_[7]) == 0; // led panel switch down
}