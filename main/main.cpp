// SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD *
// SPDX-License-Identifier: CC0-1.0

#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

#include "BYJ48Stepper.hpp"

#include <cstdlib>

#include "driver/gpio.h" //permet de paramettrer les gpio en tant qu'input/output, pupllup/pulldown

// set GPIO pin
constexpr gpio_num_t MOTOR_PIN_1 = GPIO_NUM_1;
constexpr gpio_num_t MOTOR_PIN_2 = GPIO_NUM_5;
constexpr gpio_num_t MOTOR_PIN_3 = GPIO_NUM_6;
constexpr gpio_num_t MOTOR_PIN_4 = GPIO_NUM_7;

constexpr gpio_num_t BUILD_IN_LED_PIN = GPIO_NUM_48;

constexpr gpio_num_t MINUTE_SWITCH_UP_PIN = GPIO_NUM_21;
constexpr gpio_num_t MINUTE_SWITCH_DOWN_PIN = GPIO_NUM_17;
// constexpr gpio_num_t HOUR_SWITCH_UP_PIN = GPIO_NUM_X;
// constexpr gpio_num_t HOUR_SWITCH_DOWN_PIN = GPIO_NUM_X;
// constexpr gpio_num_t DAY_SWITCH_UP_PIN = GPIO_NUM_X;
// constexpr gpio_num_t DAY_SWITCH_DOWN_PIN = GPIO_NUM_X;

// déclaration d'un struct de type gpio_config_t qui permettra de configurer les
// gpio
gpio_config_t SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION = {};
gpio_config_t BUILD_IN_LED_GPIO_CONFIGURATION = {};

// set pin number
int MOTOR_STEP_NUMBER = 2048;

// sert à faire faire le liens entre C++ et C (Esp-IDF est à la base prévu pour
// C)
extern "C" {
void app_main(void);
}

void app_main(void) { // fonction principale

  // configuration des attributs du struct de configuration des gpio
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pin_bit_mask =
      (1ULL << MINUTE_SWITCH_UP_PIN) |
      (1ULL << MINUTE_SWITCH_DOWN_PIN); // bitmask
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pull_down_en = GPIO_PULLDOWN_DISABLE;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pull_up_en = GPIO_PULLUP_ENABLE;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.mode = GPIO_MODE_INPUT;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.intr_type = GPIO_INTR_DISABLE;

  BUILD_IN_LED_GPIO_CONFIGURATION.mode = GPIO_MODE_OUTPUT;

  // la fonction gpio_congig utilise l'adresse du struct de configuration.
  gpio_config(&SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION);
  gpio_config(&BUILD_IN_LED_GPIO_CONFIGURATION);
  // attention, l'ordre des arguments des pin de moteurs doit être 1 3 2 4
  //(le 3 et le 2 sont inversés)
  FourPinStepper MyStepper1(MOTOR_STEP_NUMBER, MOTOR_PIN_1, MOTOR_PIN_3,
                            MOTOR_PIN_2, MOTOR_PIN_4);

  while (true) {
    bool ROTATION_DIRECTION =
        gpio_get_level(MINUTE_SWITCH_DOWN_PIN); // c'est la valeur sur la pin 17
                                                // qui donne le sens de rotation
    bool SHOULD_RESET = gpio_get_level(MINUTE_SWITCH_UP_PIN);

    if (SHOULD_RESET == 1 && ROTATION_DIRECTION == 1) {
      MyStepper1.ResetStep();
      gpio_set_level(BUILD_IN_LED_PIN, 1);

      printf("SHOULD_RESET is: %d \n", SHOULD_RESET);

    } else {
      MyStepper1.Step(
          ROTATION_DIRECTION); // qu'importe si 0 ou 1. tourne ds même sens
      printf("ROTATION_DIRECTION is: %d \n", ROTATION_DIRECTION);
    }

    vTaskDelay(pdMS_TO_TICKS(160)); // attend x ms
  }
}