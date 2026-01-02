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
#include "esp_timer.h"

#include <cstdlib>

#include "driver/gpio.h" //permet de paramettrer les gpio en tant qu'input/output, pupllup/pulldown

// tag pour les messages de debug
const char *TAG = "MAIN";

// defining the motor state using enums
enum class motor_state {
  MOTOR_STATE_DEFAULT = 0,
  MOTOR_STATE_CW = 1,
  MOTOR_STATE_CCW = 2,
  MOTOR_STATE_RESET = 3
};

// fonction pour obtenir le nom de l'état du moteur et pas la valeur de
// l'enum
const char *get_mode_name(motor_state mode) {
  switch (mode) {
  case motor_state::MOTOR_STATE_DEFAULT:
    return "OFF";
  case motor_state::MOTOR_STATE_CW:
    return "CLOCKWISE";
  case motor_state::MOTOR_STATE_CCW:
    return "COUNTER-CLOCKWISE";
  case motor_state::MOTOR_STATE_RESET:
    return "RESET";
  default:
    return "UNKNOWN";
  }
}

// couleur de la led build in de l'ESp32 en hexadecimal
enum class build_in_led_color {
  MOTOR_COLOR_WHITE = 0x222222,
  MOTOR_COLOR_RED = 0x220000,
  MOTOR_COLOR_GREEN = 0x002200,
  MOTOR_COLOR_BLUE = 0x000022
};

struct RGB {
  int Red, Green, Blue;
};

int Bitmask = 0b1111;

void HexToRGB(int hexValue,
              RGB *My_Color_Struct) { // la fonction n'a pas de type de return,
                                      // comme on est
  // en pass by value, on donne un pointeur pour qu'elle modifie
  // directement les valeurs de la structure color.
  if (My_Color_Struct == nullptr)
    return;
  // On décale de 16 bits pour isoler le Rouge (RRxxxx)
  My_Color_Struct->Red = (hexValue >> 16) & Bitmask; // on aurait aussi pu
                                                     // utiliser 0b11111111

  // On décale de 8 bits pour isoler le Vert (xxVVxx)
  My_Color_Struct->Green = (hexValue >> 8) & Bitmask;
  // Pas de décalage nécessaire pour le Bleu (xxxxBB)
  My_Color_Struct->Blue = hexValue & Bitmask;
}

// set GPIO pin
constexpr gpio_num_t MOTOR_1_PIN_1 = GPIO_NUM_1;
constexpr gpio_num_t MOTOR_1_PIN_2 = GPIO_NUM_5;
constexpr gpio_num_t MOTOR_1_PIN_3 = GPIO_NUM_6;
constexpr gpio_num_t MOTOR_1_PIN_4 = GPIO_NUM_7;

constexpr gpio_num_t MOTOR_2_PIN_1 = GPIO_NUM_8;
constexpr gpio_num_t MOTOR_2_PIN_2 = GPIO_NUM_10;
constexpr gpio_num_t MOTOR_2_PIN_3 = GPIO_NUM_9;
constexpr gpio_num_t MOTOR_2_PIN_4 = GPIO_NUM_14;

constexpr gpio_num_t MOTOR_3_PIN_1 = GPIO_NUM_11;
constexpr gpio_num_t MOTOR_3_PIN_2 = GPIO_NUM_13;
constexpr gpio_num_t MOTOR_3_PIN_3 = GPIO_NUM_12;
constexpr gpio_num_t MOTOR_3_PIN_4 = GPIO_NUM_4;

constexpr gpio_num_t BUILD_IN_LED_PIN =
    GPIO_NUM_47; // pourquoi le numéro n'a pas d'influence?

constexpr gpio_num_t MINUTE_SWITCH_UP_PIN = GPIO_NUM_21;
constexpr gpio_num_t MINUTE_SWITCH_DOWN_PIN = GPIO_NUM_17;
constexpr gpio_num_t HOUR_SWITCH_UP_PIN = GPIO_NUM_34;
constexpr gpio_num_t HOUR_SWITCH_DOWN_PIN = GPIO_NUM_35;
constexpr gpio_num_t DAY_SWITCH_UP_PIN = GPIO_NUM_33;
constexpr gpio_num_t DAY_SWITCH_DOWN_PIN = GPIO_NUM_36;

// déclaration struct type gpio_config_t pr configurer gpio
gpio_config_t SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION =
    {}; // defini dans void app_main(void) {

constexpr int kLoopDelayMs = 12; // delay time in ms for the main loop
constexpr int kMotorStepNumber = 2048;
// 60 * 10^6 = 60 microsecondes pour faire un tour.
constexpr int64_t kMotorMinuteFrequency = 60e6; // 60 * 10^6
constexpr int64_t kMotorHourFrequency = kMotorMinuteFrequency * 60;
constexpr int64_t kMotorDayFrequency = kMotorHourFrequency * 24;

int LastPosition = 0;
int last_Time_Hour = 0;
int last_Time_Day = 0;

// LED_STRIP configuration
#define LED_STRIP_USE_DMA 0
#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_MEMORY_BLOCK_WORDS 0
// GPIO assignment
#define LED_STRIP_GPIO_PIN 47
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

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

class Clockwheel {

public:
  Clockwheel(gpio_num_t Switch_up, gpio_num_t Switch_down,
             int64_t MotorUpdateFrequency, int kMotorStepNumber,
             gpio_num_t motor_pin_0, gpio_num_t motor_pin_1,
             gpio_num_t motor_pin_2, gpio_num_t motor_pin_3)
      : motor_(kMotorStepNumber, motor_pin_0, motor_pin_1, motor_pin_2,
               motor_pin_3),
        Switch_up_(Switch_up), Switch_dow_(Switch_down),
        MotorUpdateFrequency_(MotorUpdateFrequency) {};

  void Update(int64_t current_time) {
    SetMotorState();
    OperateMotor(current_time);
  };

private:
  FourPinStepper motor_; // composition, clockwheel HAS A motor

  int LastPosition = 0;
  gpio_num_t Switch_up_;
  gpio_num_t Switch_dow_;
  int64_t MotorUpdateFrequency_;

  // initialisé lors de la création de l'objet.
  motor_state Current_Mode = motor_state::MOTOR_STATE_DEFAULT;
  motor_state previous_Mode = motor_state::MOTOR_STATE_DEFAULT;

  void SetMotorState() {

    // la valeur lue sur la pin
    // donne le sens de rotation
    bool clockwiseRotation = gpio_get_level(Switch_dow_);
    bool shouleReset = gpio_get_level(Switch_up_);

    // set up state of the motor: stop, clockwise, counter-clockwise,
    //  reset position
    if (shouleReset == 1 && clockwiseRotation == 1) {
      Current_Mode = motor_state::MOTOR_STATE_DEFAULT;
    } else { // turn motor CW or anti CW
      if (clockwiseRotation == 0) {
        Current_Mode = motor_state::MOTOR_STATE_CW;
      } else {
        Current_Mode = motor_state::MOTOR_STATE_CCW;
      }
    }
  }

  void OperateMotor(int64_t current_time) {

    // on récupère le nombre de step pour un tour du moteur qui a été transmis à
    // la classe Stepper. Comme la classe FourPinStepper en est dérivée, on peut
    // appeler la méthode depuis cette dernière
    const int kGettedMotorStepNumber_ = motor_.get_steps_per_rotation();
    // On prend le temps pour un tour et on le divise par le nombre de pas. ça
    // nous donne le temps pour 1 pas. On divise le temps actuel par le temps
    // pour 1 pas, ce qui donne le nombre de pas actuel. On utilise un modulo
    // pour garder cette valeur entre 0 et 2048. A/B/C = A/(B*C), on va utiliser
    // la 2ème formule pour éviter la perte de précision d'une double division
    int MotorCurrentStep =
        (current_time * kGettedMotorStepNumber_ / MotorUpdateFrequency_) %
        kGettedMotorStepNumber_;

    switch (Current_Mode) {

    case (motor_state::MOTOR_STATE_RESET):
      motor_.ResetStep();
      break;
    case (motor_state::MOTOR_STATE_CW):
      motor_.Step(true);
      break;
    case (motor_state::MOTOR_STATE_CCW):
      motor_.Step(false);
      break;
    case (motor_state::MOTOR_STATE_DEFAULT):
      if (MotorCurrentStep != LastPosition) {
        motor_.Step(true);
      }
      break;
    }
    LastPosition = MotorCurrentStep;
    previous_Mode = Current_Mode;
  }
};

QueueHandle_t
    queueEvents; // permet aux fonction à qui on passe cet handle de manipuler
                 // la queue, similaire à ce qu'on a vu avec les taskhandle.

// création des instances de clockwheel
Clockwheel minuteWheel(MINUTE_SWITCH_UP_PIN, MINUTE_SWITCH_DOWN_PIN,
                       kMotorStepNumber, kMotorMinuteFrequency MOTOR_1_PIN_1,
                       MOTOR_1_PIN_3, MOTOR_1_PIN_2, MOTOR_1_PIN_4);
Clockwheel hourWheel(HOUR_SWITCH_UP_PIN, HOUR_SWITCH_DOWN_PIN, kMotorStepNumber,
                     kMotorHourFrequency MOTOR_2_PIN_1, MOTOR_2_PIN_3,
                     MOTOR_2_PIN_2, MOTOR_2_PIN_4);
// Clockwheel dayWheel(DAY_SWITCH_UP_PIN, DAY_SWITCH_DOWN_PIN,
// kMotorStepNumber,
//                     kMotorDayFrequency MOTOR_3_PIN_1, MOTOR_3_PIN_3,
//                     MOTOR_3_PIN_2, MOTOR_3_PIN_4);

void Motor_Task(void *pvParameter) {

  while (true) {
    // temps depuis le démarrage en microsecondes
    int64_t current_time = esp_timer_get_time();
    minuteWheel.Update(current_time);
    hourWheel.Update(current_time);
    // dayWheel.Update(current_time);
    vTaskDelay(pdMS_TO_TICKS(kLoopDelayMs)); // attendre 12ms
  }
}

//}
// sert à faire faire le liens entre C++ et C (Esp-IDF est à la base prévu
// pour C)
extern "C" {
void app_main(void);
}

void app_main(void) { // fonction principale

  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.intr_type = GPIO_INTR_DISABLE;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.mode = GPIO_MODE_INPUT;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pin_bit_mask =
      (1ULL << MINUTE_SWITCH_UP_PIN) | (1ULL << MINUTE_SWITCH_DOWN_PIN);
  // 1ull signifie 1 en binaire, U veut dire unsign donc forcément positif
  // LL veut dire Long Long (64 bits). On combine les masque grâce à |

  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pull_down_en = GPIO_PULLDOWN_DISABLE;
  SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK(gpio_config(&SWITCH_BUTTON_MOTOR_GPIO_CONFIGURATION));
  // ESP_ERROR_CHECK arrête le programme si retourne un message d'erreur autre
  // que ESP-OK. gpio_config paramêtre les gpio selon le struct qu'on lui
  // passe en argument

  queueEvents = xQueueCreate(1, sizeof(int));
  // creation de la queue avec 1 message max, chaque message = 1 int

  xTaskCreate(&Motor_Task, "Motor_Task", 4096, NULL, 5, NULL);
}

//-----------------SUITE PROGRAMME PRINCIPAL-----------------------------

// renommer selon convention

// la led de status est une classe séparée.

// on donne l'adresse de color pour que la fonction puisse modifier les
// valeurs à l'intérieur
HexToRGB(static_cast<int>(build_in_led_color::MOTOR_COLOR_WHITE), &color);

// update led strip with the new values
ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, color.Red, color.Green,
                                    color.Blue));
ESP_ERROR_CHECK(led_strip_refresh(led_strip));

// préparer fonction de log pour debugger la classe clockwheel
if (previous_Mode != Current_Mode) {
  ESP_LOGI(TAG, "MOTOR_STATE : %s", get_mode_name(Current_Mode));
  ESP_LOGI(TAG, "clockwiseRotation is: %d \n", clockwiseRotation);
}

//---ledstrip status led task...............
// start ledstrip
led_strip_handle_t led_strip = configure_led();
RGB color{0, 0, 0};
int offset = 0;

// les 3 derniers commits ont étés fait dans main mais j'aurais du les faire
// dans la branch. 2.1.25 19h04
