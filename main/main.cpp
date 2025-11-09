// SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD *
// SPDX-License-Identifier: CC0-1.0

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

#include "BYJ48Stepper.h"

#include <cstdlib>

#include "driver/gpio.h"

// set GPIO pin
constexpr gpio_num_t MOTOR_PIN_1 = GPIO_NUM_2;
constexpr gpio_num_t MOTOR_PIN_2 = GPIO_NUM_6;
constexpr gpio_num_t MOTOR_PIN_3 = GPIO_NUM_8;
constexpr gpio_num_t MOTOR_PIN_4 = GPIO_NUM_10;

// set pin number
int MOTOR_STEP_NUMBER = 2048;

void app_main(void) { // fonction principale

  Stepper MyStepperTemplate(MOTOR_STEP_NUMBER);

  FourPinStepper MyStepper1(MOTOR_PIN_1, MOTOR_PIN_2, MOTOR_PIN_3, MOTOR_PIN_4);

  while (true) {
    MyStepper1.Step();
    vTaskDelay(pdMS_TO_TICKS(500)); // 500 ms
  }
}