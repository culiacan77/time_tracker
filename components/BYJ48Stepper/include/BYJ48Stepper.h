#ifndef BYJ48Stepper_h
#define BYJ48Stepper_h
#include "driver/gpio.h"

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

class FourPinStepper : public Stepper {
public:
  FourPinStepper(int steps_per_rotation, gpio_num_t motor_pin_0,
                 gpio_num_t motor_pin_1, gpio_num_t motor_pin_2,
                 gpio_num_t motor_pin_3)
      : Stepper(steps_per_rotation) {
    motor_pins_[0] = motor_pin_0;
    motor_pins_[1] = motor_pin_1;
    motor_pins_[2] = motor_pin_2;
    motor_pins_[3] = motor_pin_3;
  }

  void Step(bool clockwise = true) override;

private:
  gpio_num_t motor_pins_[4];
  static int sequence_array_[4];
};

#endif
