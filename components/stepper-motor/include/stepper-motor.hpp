// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#pragma once

/**
 * Abstract interface for stepper motor drivers.
 */
class StepperMotor {
   public:
    /**
     * Create a stepper motor with a given number of steps per full rotation.
     */
    StepperMotor(int steps_per_rotation);

    /**
     * Move the motor by one step.
     * @param clockwise true to step clockwise, false for counter-clockwise.
     */
    virtual void Step(bool clockwise = true);

    /**
     * Move the motor on step towards the zero position.
     * @return number of steps moved:
     *    1 if stepped clockwise,
     *   -1 if stepped counter-clockwise,
     *    0 if already at zero position.
     */
    int ResetStep();

    int steps_per_rotation_;
    int current_step_;
};
