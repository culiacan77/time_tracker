// SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD *
// SPDX-License-Identifier: CC0-1.0

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "led_strip.h"
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

constexpr gpio_num_t BUILD_IN_LED_PIN =
    GPIO_NUM_47; // pourquoi le numéro n'a pas d'influence?

constexpr gpio_num_t MINUTE_SWITCH_UP_PIN = GPIO_NUM_21;
constexpr gpio_num_t MINUTE_SWITCH_DOWN_PIN = GPIO_NUM_17;
// constexpr gpio_num_t HOUR_SWITCH_UP_PIN = GPIO_NUM_X;
// constexpr gpio_num_t HOUR_SWITCH_DOWN_PIN = GPIO_NUM_X;
// constexpr gpio_num_t DAY_SWITCH_UP_PIN = GPIO_NUM_X;
// constexpr gpio_num_t DAY_SWITCH_DOWN_PIN = GPIO_NUM_X;

// déclaration d'un struct de type gpio_config_t qui permettra de configurer les
// gpio
gpio_config_t SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION =
    {}; // defini dans void app_main(void) {

// set pin number
int MOTOR_STEP_NUMBER = 2048;

//  attention, l'ordre des arguments des pin de moteurs doit être 1 3 2 4
//(le 3 et le 2 sont inversés)
FourPinStepper MyStepper1(MOTOR_STEP_NUMBER, MOTOR_PIN_1, MOTOR_PIN_3,
                          MOTOR_PIN_2, MOTOR_PIN_4);

QueueHandle_t
    queueEvents; // permet aux fonction à qui on passe cet handle de manipuler
                 // la queue, similaire à ce qu'on a vu avec les taskhandle.

// LED_STRIP configuration
#define LED_STRIP_USE_DMA 0
#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_MEMORY_BLOCK_WORDS 0
// GPIO assignment
#define LED_STRIP_GPIO_PIN 47
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)
static const char *TAG = "main.cpp";

led_strip_handle_t configure_led(void) {
  // LED strip general initialization, according to your led board design
  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the
                                            // LED strip's data line
      .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
      .led_model = LED_MODEL_WS2812,        // LED strip model
      .color_component_format =
          LED_STRIP_COLOR_COMPONENT_FMT_RGB, // The color order of the strip:
                                             // RGB
      .flags = {
          .invert_out = false, // don't invert the output signal
      }};

  // LED strip backend configuration: RMT
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to
                                      // different power consumption
      .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
      .mem_block_symbols =
          LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT
                                        // channel
      .flags = {
          .with_dma = LED_STRIP_USE_DMA, // Using DMA can improve performance
                                         // when driving more LEDs
      }};

  // LED Strip object handle
  led_strip_handle_t led_strip;
  ESP_ERROR_CHECK(
      led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  ESP_LOGI(TAG, "Created LED strip object with RMT backend");
  return led_strip;
}

void led_task(void *pvParameter) {
  // start ledstrip
  led_strip_handle_t led_strip = configure_led();
  int CASE_COLOR;
  int CURRENT_COLOR =
      0; // initaialisé à une valeur n'existant pas dans CASE_COLOR.

  while (1) {
    // si un message est dans la queue
    if (xQueueReceive(queueEvents, &CASE_COLOR, portMAX_DELAY) == pdPASS) {

      for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
        if (CASE_COLOR == 1) {
          ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 10, 0));
        } else if (CASE_COLOR == 2) {
          ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 10, 0, 0));
        } else if (CASE_COLOR == 3) {
          ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 0, 10));
        }
      }
      // 3. Rafraîchir la LED (UNIQUEMENT APRÈS avoir reçu un événement)
      ESP_ERROR_CHECK(led_strip_refresh(led_strip));
      vTaskDelay(pdMS_TO_TICKS(500)); // delay pour voir le changement de
                                      // couleur
      if (CASE_COLOR !=
          CURRENT_COLOR) { // trigger log only when CASE_COLOR changes
        ESP_LOGI(TAG, "queue received, case: %d \n", CASE_COLOR);
        CURRENT_COLOR = CASE_COLOR;
      }
    }
  }
}
// end ledstrip


// sert à faire faire le liens entre C++ et C (Esp-IDF est à la base prévu
// pour C)
extern "C" {
void app_main(void);
}

void app_main(void) { // fonction principale

  // la fonction gpio_congig utilise l'adresse du struct de configuration.
  gpio_config(&SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION);

  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.intr_type = GPIO_INTR_DISABLE;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.mode = GPIO_MODE_INPUT;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pin_bit_mask =
      (1ULL << MINUTE_SWITCH_UP_PIN) |
      (1ULL << MINUTE_SWITCH_DOWN_PIN) // le | va combiner les masques, 2 0
                                       // reste 0. 0 et 1 deveiennent 1
      ;                                // le | est un "ou" qui rajoute les bits
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pull_down_en = GPIO_PULLDOWN_DISABLE;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK(gpio_config(
      &SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION)); // arrête le programme si
                                                 // retourne un message d'erreur
                                                 // autre que ESP-OK

  queueEvents =
      xQueueCreate(1, sizeof(int)); // creation de la queue avec 1 message
                                    // max, chaque message = 1 int

  xTaskCreate(&led_task, "led_task", 4096, NULL, 5, NULL);
while (true) {
    int MOTOR_CASE;
    bool ROTATION_DIRECTION =
        gpio_get_level(MINUTE_SWITCH_DOWN_PIN); // c'est la valeur sur la pin 17
                                                // qui donne le sens de rotation
    bool SHOULD_RESET = gpio_get_level(MINUTE_SWITCH_UP_PIN);

    // reset position
    if (SHOULD_RESET == 1 && ROTATION_DIRECTION == 1) {
      MyStepper1.ResetStep();
      ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 0, 10, 0));
      MOTOR_CASE = 1;
      ESP_LOGI(TAG, "Reseting position \n");

    } else { // turn motor CW or anti CW
      MyStepper1.Step(ROTATION_DIRECTION);
      if (ROTATION_DIRECTION == 0) {
        MOTOR_CASE = 2;
      } else {
        MOTOR_CASE = 3;
      }
      ESP_LOGI(TAG, "ROTATION_DIRECTION is: %d \n", ROTATION_DIRECTION);
    }
    // Send should_reset_bool value to queue
    xQueueSend(queueEvents, &MOTOR_CASE, portMAX_DELAY);
    ESP_LOGI(TAG, "MOTOR_CASE is: %d \n", MOTOR_CASE);
    vTaskDelay(pdMS_TO_TICKS(250)); // Exemple: 10ms
  }
}