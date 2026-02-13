// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include <inttypes.h>

#include <vector>

#include "clockwheel.hpp" //pk pas besoin de include Clockwheel.cpp ?
#include "driver/gpio.h" //permet de paramettrer les gpio en tant qu'input/output, pupllup/pulldown
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "led_panel.hpp"
#include "led_strip.h"
#include "sdkconfig.h"
#include "stepper-motor-4p.hpp"

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

constexpr gpio_num_t kLedPanelPin = GPIO_NUM_38;

constexpr int kLedStripRmtResHz = (10 * 1000 * 1000);
constexpr int kLoopDelayMs = 12; // delay time in ms for the main loop
constexpr int kMotorStepNumber = 2048;

constexpr int64_t kMotorMinuteFrequency = 60e6; // 60 * 10^6
constexpr int64_t kMotorHourFrequency = kMotorMinuteFrequency * 60;
constexpr int64_t kMotorDayFrequency = kMotorHourFrequency * 24;

constexpr int kUpdateDelayMs = 50; // delay time in ms for the led panel update

// liste position des leds à allumer selon le pattern
// mode pause
static const std::vector<std::vector<int>> kLightPatternStop = {
    {0, 0}, {1, 0}, {2, 0}, {0, 2}, {1, 2}, {2, 2}};
// mode normal + rainbow (sans fade pr full lit)
static const std::vector<std::vector<int>> kLightPatternRun = {
    {0, 0}, {0, 1}, {0, 2}, {1, 2}, {1, 1}, {1, 0}, {2, 0}, {2, 1}, {2, 2}};
// mode reset
static const std::vector<std::vector<int>> kLightPatternReset = {
    {0, 1}, {0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}};

static const std::vector<std::vector<int>> kFullLit = {
    {0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}};

bool shouldDimLight =
    true; // variable globale pour indiquer si les LEDs doivent être atténuées

extern "C" {
void app_main(void);
}

void app_main(void) {
  // --- Initialisation de la LED de status ---
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
  LedPanel myLedPanel(kLedPanelPin, &rmt_config);

  // confiure trail length and fill trail brightness vector
  myLedPanel.setTrailLength(6);
  // Appelez la fonction pour allumer le panneau avec votre pattern et le
  // handle
  myLedPanel.setAnimationPattern(kFullLit);

  // boucle test led panel
  ////while (true) {
  // myLedPanel.shouldDimLight_ = false;
  //  vTaskDelay(pdMS_TO_TICKS(100)); // provisoire. Devra être géré par la task
  //  myLedPanel.updateMatrix(shouldDimLight);
  //  myLedPanel.shiftLedColor();
  //  myLedPanel.litLedPanel();
  // };

  // boucle test moteur
  while (true) {
    current_time = esp_timer_get_time();
    minute_clock_wheel.Update(current_time);
    hour_clock_wheel.Update(current_time);
    day_clock_wheel.Update(current_time);
    vTaskDelay(pdMS_TO_TICKS(kLoopDelayMs)); // attendre 12ms
  }
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
