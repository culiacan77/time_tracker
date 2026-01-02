// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include <inttypes.h>

#include "clockwheel.hpp"
#include "driver/gpio.h"  //permet de paramettrer les gpio en tant qu'input/output, pupllup/pulldown
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "stepper-motor-4p.hpp"

// tag pour les messages de debug
const char* kTag = "main";

constexpr gpio_num_t kMinuteMotorPin1 = GPIO_NUM_1;
constexpr gpio_num_t kMinuteMotorPin2 = GPIO_NUM_5;
constexpr gpio_num_t kMinuteMotorPin3 = GPIO_NUM_6;
constexpr gpio_num_t kMinuteMotorPin4 = GPIO_NUM_7;

constexpr gpio_num_t kStatusLedPin = GPIO_NUM_47;

constexpr gpio_num_t kMinuteSwitchUpPin   = GPIO_NUM_21;
constexpr gpio_num_t kMinuteSwitchDownPin = GPIO_NUM_17;

constexpr int kLedStripRmtResHz = (10 * 1000 * 1000);
constexpr int kLoopDelayMs      = 12;  // delay time in ms for the main loop
constexpr int kMotorStepNumber  = 2048;

constexpr int64_t kMotorMinuteFrequency = 60e6;  // 60 * 10^6
constexpr int64_t kMotorHourFrequency   = kMotorMinuteFrequency * 60;
constexpr int64_t kMotorDayFrequency    = kMotorHourFrequency * 24;

//  attention, l'ordre des arguments des pin de moteurs doit être 1 3 2 4
//(le 3 et le 2 sont inversés)

extern "C" {
void app_main(void);
}

void app_main(void) {
    led_strip_config_t strip_config     = {};
    strip_config.strip_gpio_num         = kStatusLedPin;
    strip_config.max_leds               = 1;
    strip_config.led_model              = LED_MODEL_WS2812;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.clk_src                = RMT_CLK_SRC_DEFAULT;
    rmt_config.resolution_hz          = kLedStripRmtResHz;

    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(kTag, "Created LED strip object with RMT backend");

    int64_t current_time = esp_timer_get_time();
    StepperMotor4P minute_stepper_motor(
        kMotorStepNumber, kMinuteMotorPin1, kMinuteMotorPin3, kMinuteMotorPin2, kMinuteMotorPin4);
    ClockWheel minute_clock_wheel(kMinuteSwitchUpPin,
                                  kMinuteSwitchDownPin,
                                  kMotorMinuteFrequency,
                                  minute_stepper_motor,
                                  current_time,
                                  led_strip);

    while (true) {
        current_time = esp_timer_get_time();
        minute_clock_wheel.Update(current_time);
        vTaskDelay(pdMS_TO_TICKS(kLoopDelayMs));  // attendre 12ms
    }
}
