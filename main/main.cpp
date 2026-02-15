// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include <inttypes.h>

#include <vector>

#include "clockwheel.hpp" //pk pas besoin de include Clockwheel.cpp ?
#include "driver/gpio.h" //permet de paramettrer les gpio en tant qu'input/output, pupllup/pulldown
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h" //fonctions de freeRTOS
#include "freertos/task.h"     //fonctions de freeRTOS
#include "led_panel.hpp"
#include "led_strip.h"
#include "sdkconfig.h"
#include "stepper-motor-4p.hpp"
#include <stdio.h>

// tag pour les messages de debug
static const char *kTag = "main";

// fonction pour obtenir le nom de l'état du moteur et pas la valeur de
// l'enum
// const char *get_mode_name(motor_state mode) {
// switch (mode) {
// case motor_state::MOTOR_STATE_DEFAULT:
//   return "OFF";
// case motor_state::MOTOR_STATE_CW:
//   return "CLOCKWISE";
// case motor_state::MOTOR_STATE_CCW:
//   return "COUNTER-CLOCKWISE";
// case motor_state::MOTOR_STATE_RESET:
//   return "RESET";
// default:
//   return "UNKNOWN";
// }
//}

constexpr gpio_num_t kMinuteMotorPin1 = GPIO_NUM_1;
constexpr gpio_num_t kMinuteMotorPin2 = GPIO_NUM_5;
constexpr gpio_num_t kMinuteMotorPin3 = GPIO_NUM_6;
constexpr gpio_num_t kMinuteMotorPin4 = GPIO_NUM_7;

constexpr gpio_num_t kHourMotorPin1 = GPIO_NUM_8;
constexpr gpio_num_t kHourMotorPin2 = GPIO_NUM_10;
constexpr gpio_num_t kHourMotorPin3 = GPIO_NUM_9;
constexpr gpio_num_t kHourMotorPin4 = GPIO_NUM_14;

constexpr gpio_num_t kDayMotorPin1 = GPIO_NUM_11;
constexpr gpio_num_t kDayMotorPin2 = GPIO_NUM_13;
constexpr gpio_num_t kDayMotorPin3 = GPIO_NUM_12;
constexpr gpio_num_t kDayMotorPin4 = GPIO_NUM_4;

constexpr gpio_num_t kStatusLedPin = GPIO_NUM_47;

constexpr gpio_num_t kMinuteSwitchUpPin = GPIO_NUM_21;
constexpr gpio_num_t kMinuteSwitchDownPin = GPIO_NUM_17;
constexpr gpio_num_t kHourSwitchUpPin = GPIO_NUM_34;
constexpr gpio_num_t kHourSwitchDownPin = GPIO_NUM_35;
constexpr gpio_num_t kDaySwitchUpPin = GPIO_NUM_33;
constexpr gpio_num_t kDaySwitchDownPin = GPIO_NUM_36;

constexpr gpio_num_t kLedPanelSwitchUp = GPIO_NUM_16;
constexpr gpio_num_t kLedPanelSwitchDown = GPIO_NUM_18;

constexpr gpio_num_t kLedPanelPin = GPIO_NUM_38;

constexpr int kLedStripRmtResHz = (10 * 1000 * 1000);
constexpr int kLoopDelayMs = 12; // delay time in ms for the main loop
constexpr int kMotorStepNumber = 2048;

constexpr int64_t kMotorMinuteFrequency = 60e6; // 60 * 10^6
constexpr int64_t kMotorHourFrequency = kMotorMinuteFrequency * 60;
constexpr int64_t kMotorDayFrequency = kMotorHourFrequency * 24;

constexpr int kUpdateDelayMs = 50; // delay time in ms for the led panel update

static constexpr int kLedPanelLeds = 9; // nbr LEDs dans le panneau

bool shouldDimLight =
    true; // variable globale pour indiquer si les LEDs doivent être atténuées

//  --------TASK DEFINITION--------
void ledPanel_Task(void *pvParameter) {

  // on peut initialiser les variables utiles à LedPanel ici, pareil pour la
  // création de l'objet.

  // ---------------configuration du LED PANEL----------------
  led_strip_rmt_config_t rmt_config = {};
  rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
  rmt_config.resolution_hz = kLedStripRmtResHz;

  led_strip_config_t ledPanel_strip_config = {};
  ledPanel_strip_config.strip_gpio_num = kLedPanelPin;
  ledPanel_strip_config.max_leds =
      kLedPanelLeds; // Utilisez le bon nombre de LEDs
  ledPanel_strip_config.led_model =
      LED_MODEL_WS2812; // Assurez-vous du bon modèle
  ledPanel_strip_config.color_component_format =
      LED_STRIP_COLOR_COMPONENT_FMT_RGB;

  // led_strip_rmt_config_t rmt_config  similaire à celui de status led utilisé
  // par Clockwheel

  led_strip_handle_t ledPanel_led_strip;

  ESP_ERROR_CHECK(led_strip_new_rmt_device(&ledPanel_strip_config, &rmt_config,
                                           &ledPanel_led_strip));

  LedPanel myLedPanel(kLedPanelSwitchUp, kLedPanelSwitchDown, kLedPanelPin,
                      ledPanel_led_strip);
  // confiure trail length and fill trail brightness vector
  myLedPanel.setTrailLength(6);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(100)); // provisoire. Devra être géré par la task
    myLedPanel.update();
    myLedPanel.updateMatrix(true); // remplacer true par shouldDimLight, ajout
                                   // en tant que parametre de task ?
    myLedPanel.litLedPanel();
  }
}

void motor_Task(void *pvParameter) {
  // ------------------configuration CLOCKWHEEL------------------
  // --- Initialisation de la LED de status clockheel--
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = kStatusLedPin;
  strip_config.max_leds = 1;
  strip_config.led_model = LED_MODEL_WS2812;
  strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
  rmt_config.resolution_hz = kLedStripRmtResHz;

  led_strip_handle_t led_strip;

  ESP_ERROR_CHECK(led_strip_new_rmt_device(
      &strip_config, &rmt_config,
      &led_strip)); // assigne les paramètre de led_strip_config et
                    // rmt_config à led_strip
  ESP_LOGI(kTag, "Created LED strip object with RMT backend");

  int64_t current_time = esp_timer_get_time();

  StepperMotor4P minute_stepper_motor(kMotorStepNumber, kMinuteMotorPin1,
                                      kMinuteMotorPin3, kMinuteMotorPin2,
                                      kMinuteMotorPin4);
  ClockWheel minute_clock_wheel(kMinuteSwitchUpPin, kMinuteSwitchDownPin,
                                kMotorMinuteFrequency, minute_stepper_motor,
                                current_time, led_strip);

  StepperMotor4P hour_stepper_motor(kMotorStepNumber, kHourMotorPin1,
                                    kHourMotorPin3, kHourMotorPin2,
                                    kHourMotorPin4);
  ClockWheel hour_clock_wheel(kHourSwitchUpPin, kHourSwitchDownPin,
                              kMotorHourFrequency, hour_stepper_motor,
                              current_time);

  StepperMotor4P day_stepper_motor(kMotorStepNumber, kDayMotorPin1,
                                   kDayMotorPin3, kDayMotorPin2, kDayMotorPin4);
  ClockWheel day_clock_wheel(kDaySwitchUpPin, kDaySwitchDownPin,
                             kMotorDayFrequency, day_stepper_motor,
                             current_time);

  while (1) {
    current_time = esp_timer_get_time();
    minute_clock_wheel.Update(current_time);
    hour_clock_wheel.Update(current_time);
    day_clock_wheel.Update(current_time);
    vTaskDelay(pdMS_TO_TICKS(kLoopDelayMs)); // attendre 12ms
  }
}
extern "C" {
void app_main(void);
}

void app_main(void) {

  // ---------------creation des tasks----------------
  xTaskCreate(&motor_Task, "motor_Task", 4096, NULL, 5, NULL);
  xTaskCreate(&ledPanel_Task, "ledPanel_Task", 4096, NULL, 4, NULL);
}
// clockwheel n est pas une tache freertos.
//  switch up = turn cw | neutral normal | down = turn ccw | 3 Up = reset
//  utiliser freertos queue pour communiquer etat reset à led panel
//  créer task ledpanel
//  créer objet led panel à partir de class led strip ? ou créer class puis
//  objet led panel ? switch led panel: left: pause device | center: run normal|
//  right: change led color rainbow. attention. en mode pause les clockwheels en
//  position neutre arretent de tourner.
//  reimprimer couvercle. ne pas y mettre d'inser.
//  souder chargeur.
//  assembler le tout.

//----------QUESTIONEMENT------------------
// le fichier cmakelists.txt du dossier led-panel doit il inclure led_strip.h ?
// je crois pas

// ajouter le ledhandle pour le ledpanel

// xxxsafestate
