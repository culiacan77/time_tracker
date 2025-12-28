#ifndef BYJ48STEPPER_H
#define BYJ48STEPPER_H

#include "driver/gpio.h"

// ================== Classe de base ==================
class Stepper {
public:
  Stepper(int steps_per_rotation);
  virtual void Step(bool clockwise = true) = 0;

  bool ResetStep();
  int CurrentStep();

protected:
  int current_step_;
  int steps_per_rotation_;
};

// ================== Classe 4-pin ==================
class FourPinStepper : public Stepper {
public:
  FourPinStepper(int steps_per_rotation, gpio_num_t motor_pin_0,
                 gpio_num_t motor_pin_1, gpio_num_t motor_pin_2,
                 gpio_num_t motor_pin_3);

  void Step(bool clockwise = true) override;

private:
  gpio_num_t motor_pins_[4];
  static int sequence_array_[4];
};

#endif // BYJ48STEPPER_H
