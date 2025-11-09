#include "BYJ48Stepper.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

// ---------------- Stepper ----------------
Stepper::Stepper(int steps_per_rotation)
    : current_step_(0), steps_per_rotation_(steps_per_rotation) {}

bool Stepper::ResetStep() {
  int exceding_steps = current_step_ % steps_per_rotation_;
  if (exceding_steps == 0)
    return false;

  bool clockwise = exceding_steps >= steps_per_rotation_ / 2;
  Step(clockwise);
  return true;
}

int Stepper::CurrentStep() { return current_step_; }

// ---------------- FourPinStepper ----------------
int FourPinStepper::sequence_array_[4] = {0b1001, 0b0110, 0b1100, 0b0011};

// 🔧 Constructeur : configure les GPIO en sortie
FourPinStepper::FourPinStepper(int steps_per_rotation, gpio_num_t motor_pin_0,
                               gpio_num_t motor_pin_1, gpio_num_t motor_pin_2,
                               gpio_num_t motor_pin_3)
    : Stepper(steps_per_rotation) {
  motor_pins_[0] = motor_pin_0;
  motor_pins_[1] = motor_pin_1;
  motor_pins_[2] = motor_pin_2;
  motor_pins_[3] = motor_pin_3;

  // Configuration des pins comme sorties
  for (int i = 0; i < 4; i++) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << motor_pins_[i]);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(motor_pins_[i], 0); // initialise à LOW
  }
}

void FourPinStepper::Step(bool clockwise) {
  current_step_ += (clockwise ? 1 : -1);

  int sequence_array_index_ = (current_step_ % 4 + 4) % 4;
  int current_sequence = FourPinStepper::sequence_array_[sequence_array_index_];

  printf("Step %d, sequence 0x%X\n", current_step_, current_sequence);

  // appliquer la séquence
  for (int i = 0; i < 4; i++) {
    int level = (current_sequence >> i) & 0x1;
    gpio_set_level(motor_pins_[i], level);
  }
}
