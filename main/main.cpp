#include "BYJ48Stepper.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Définition des pins
constexpr gpio_num_t MOTOR_PIN_1 = GPIO_NUM_2;
constexpr gpio_num_t MOTOR_PIN_2 = GPIO_NUM_6;
constexpr gpio_num_t MOTOR_PIN_3 = GPIO_NUM_8;
constexpr gpio_num_t MOTOR_PIN_4 = GPIO_NUM_10;

// Nombre de pas par rotation
constexpr int MOTOR_STEP_NUMBER = 2048;

extern "C" void app_main(void) {
  FourPinStepper MyStepper(
      MOTOR_STEP_NUMBER, MOTOR_PIN_1, MOTOR_PIN_3, MOTOR_PIN_2,
      MOTOR_PIN_4); // l'ordre des entrée du moteur est 1-3-2-4

  while (true) {
    MyStepper.Step();
    printf("Step envoyée\n");
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}
