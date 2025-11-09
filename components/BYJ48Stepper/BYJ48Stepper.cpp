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

void FourPinStepper::Step(bool clockwise) {
  current_step_ += (clockwise ? 1 : -1);

  int sequence_array_index_ = (current_step_ % 4 + 4) % 4;
  int current_sequence = FourPinStepper::sequence_array_[sequence_array_index_];

  printf("Step %d, sequence 0x%X\n", current_step_, current_sequence); // DEBUG

  // éteindre toutes les pins
  for (int i = 0; i < 4; i++) {
    gpio_set_level(motor_pins_[i], 0);
  }

  // allumer selon la séquence
  for (int i = 0; i < 4; i++) {
    if (current_sequence & (1 << i)) {
      gpio_set_level(motor_pins_[i], 1);
    }
  }
}
