/*
 * BYJ48Stepper.h - ESP IDF adaptation based on the Stepper library for
 * Wiring/Arduino - Version 1.1.0
 *
 * Source material:
 *
 * Original library       (0.1)   by Tom Igoe.
 * Two-wire modifications  (0.2)   by Sebastian Gassner
 * Combination version     (0.3)   by Tom Igoe and David Mellis
 * Bug fix for four-wire   (0.4)   by Tom Igoe, bug fix from Noah Shibley
 * High-speed stepping mod         by Eugene Kozlenko
 * Timer rollover fix              by Eugene Kozlenko
 * Five phase five wire    (1.1.0) by Ryan Orendorff
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *  * The circuits can be found at
 *
 * http://www.arduino.cc/en/Tutorial/Stepper
 *
 * Refactoring for ESP IDF         by Thomas Fesseler under Jacques Supcik
 * supervisiotn.
 *
 * The sequence of control signals for 4 control wires is as follows:
 *
 * Step C0 C1 C2 C3
 *    1  1  0  1  0
 *    2  0  1  1  0
 *    3  0  1  0  1
 *    4  1  0  0  1
 *
 */

// ensure this library description is only included once
#ifndef BYJ48Stepper_h
#define BYJ48Stepper_h

class Stepper {
public:
  // constructors:
  Stepper(int steps_per_rotation);
  virtual void
  Step(bool clockwise =
           true) = 0; // virtual (que pour méthodes pas fonctions) car on
  // va la redéfinir ailleur. = 0 vx dire que c'est abstrait = on n'a pas le
  // droit de l'appeler depuis cette classe (Stepper). on doit l'appeler depuis
  // une instance de classe dééivée

  bool ResetStep();
  int CurrentStep();

protected: // si j'avais mis private: les méthodes ne peuvent pas être appelées
  // depuis les classes filles (et l'extérieur de la classe). Les
  // attributs ne peuvent pas être redéfinis dans la classe fille où en
  // dehors de la classe.

  int current_step_;
  int steps_per_rotation_;
};

// classe enfant/dérivée héritant de la classe stepper (parent), si un
// utilisateur veut faire une class 2 pin, libre à lui.
class FourPinStepper : public Stepper {
public:
  // constructors avec liste d'initialisation. Le : introduit la liste
  // d'initialisation
  FourPinStepper(int motor_pin_0, int motor_pin_1, int motor_pin_2,
                 int motor_pin_3)
      : motor_pins_[0](motor_pin_0), motor_pins_[1](motor_pin_1),
        motor_pins_[2](motor_pin_2), motor_pins_[3](motor_pin_3) {}

  void Step() override; // annonce qu'on va redéfinire la méthode Step. Possible
                        // car déclaré virtual dans Stepper

private:
  int motor_pins_[4]; // array de 4 int

  static int sequence_array_[4];
  // partagé par toutes les instances de cette classes et les classes
  // dérivées (de cette classe) si il y en avaient
  // on définira la séquence d'allumage des coils du moteur pour le faire
  // tourner dans le .cpp
};

#endif
