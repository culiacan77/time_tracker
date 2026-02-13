// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "stepper-motor.hpp"

StepperMotor::StepperMotor(int steps_per_rotation)
    : steps_per_rotation_(steps_per_rotation), // liste d'initialisation
      current_step_(0) {};

void StepperMotor::Step(bool clockwise) {
  if (clockwise) {
    current_step_ = (current_step_ + 1) % steps_per_rotation_;
  } else {
    current_step_ =
        (current_step_ + steps_per_rotation_ - 1) % steps_per_rotation_;
  }
}

int StepperMotor::ResetStep() {
  int pos = current_step_ % steps_per_rotation_;
  if (pos == 0) {
    return 0; // already at zero position
  }
  bool cw = pos >= steps_per_rotation_ / 2;
  Step(cw);
  return cw ? 1 : -1; // opérateur terner. retrun 1 if true, -1 if false
}