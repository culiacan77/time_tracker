#include "BYJ48Stepper.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Définition des pins
constexpr gpio_num_t MOTOR_PIN_1 = GPIO_NUM_1;
constexpr gpio_num_t MOTOR_PIN_2 = GPIO_NUM_5;
constexpr gpio_num_t MOTOR_PIN_3 = GPIO_NUM_6;
constexpr gpio_num_t MOTOR_PIN_4 = GPIO_NUM_7;

// Nombre de pas par rotation
constexpr int MOTOR_STEP_NUMBER = 2048;

extern "C" void app_main(void) {

  printf("Booting... waiting for stabilization\n"); // message de debug
  vTaskDelay(pdMS_TO_TICKS(1000)); // attente de 1 sec pour ne plus avoir valeur
                                   // changeante sur straping pin
  FourPinStepper MyStepper(
      MOTOR_STEP_NUMBER, MOTOR_PIN_1, MOTOR_PIN_2, MOTOR_PIN_3,
      MOTOR_PIN_4); // ne marche pas avec ordre 1 2 3 4 // 1 3 2 4

  while (true) {
    MyStepper.Step(true);
    printf("Step envoyée\n");
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}
