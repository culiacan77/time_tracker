#include "BYJ48Stepper.h"

#include <stdlib.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// définition du constructeur de la classe Stepper
Stepper::Stepper(steps_per_rotation)
    : stepsPerRotation_(steps_per_rotation), // liste d'initialisation
      current_step_(0) {
  // initialisation de l'attribut current_step_ à 0 mieux vaut faire ça qu'une
  // attribution dans le constructeur (currentStep_ = 0) c'est une bonne
  // habitude à prendre car si on avait un const on ne pourrait pas se permette
  // que le constructeur lui donne une valeur random lors de sa création (c'est
  // ce que fait le constructeur avant la liste d'initialisation) puis lui donne
  // la valeur 0 (un const ne peut pas changer) problème similaire avec les
  // pointeurs.
}

// définition de la méthode step
void FourPinStepper::Step(bool clockwise) {
  // Mise à jour de currentStep selon le sens de rotation
  if (clockwise) {
    current_step_++; // mise à jour de current_step_ par increment de 1
  } else {
    current_step_--;
  }

  // Calcule la position dans la séquence. le fait de faire %4 +4 permet de
  // ne pas avoir de résultat négatif, tout en gardant le même résultat du
  // modulo. On aurait pu se passer du double %4 si on avait une structure avec
  // current_step_ = current_step_ +4 %4, ce qui nous aurait garanti que
  // current_step_reste compris entre 0 et 4. Mais comme on a besoin que current
  // step_ puisse devenir plus grand pour pouvoir être comparé à
  // steps_per_rotation dans la méthode ResetStep, current_step_ n'a pas de
  // limite et pourrait atteindre n'importe quelle valeur négative si
  // l'utilisateur utilise les boutons pour faire tourner un moteur en sens
  // antihoraire.
  int sequence_array_index_ = (current_step_ % 4 + 4) % 4;
}

// commentaires détaillent le cas i = 0
for (int i = 0, i < 4, i++) {
  gpio_set_level(motor_pins_[i], 0); // extinction de tous les moteurs
}

int current_sequence = sequence_array_[sequence_array_index_];

for (int i = 0, i < 4, i++) {
  if (current_sequence & (1 << i)) {
    // on récupère l'entrée du tableau selon current_sequence, ex la 1ère
    // séquence. on utilise la logique AND grâce à &. On utilise le masque
    // binaire 0001 grâce à 1. à ce stade on décalé de 0 vers la gauche donc
    // 0001. à la prochaine étape il vaudra 0010.
    gpio_set_level(motor_pins[i], 1);
  }
}

bool FourPinStepper::ResetStep() {
  int exceding_steps = current_step_ % steps_per_rotation_;
  if (exceding_steps == 0) {
    return false; // return met fin à la méthode ResetStep. le fait que cette
                  // méthode soit un bool fait qu'on doit retourner une valeur
                  // soit true soit false. ça permet de savoir si la méthode est
                  // dans un cas où exceding_steps = 0 où pas.
  }
  if (exceding_steps >= steps_per_rotation_ / 2) {
    clockwise = true; // si on a effectué plus de 50% de la rotation du moteur
                      // (comparé à la position de départ) on va continuer à
                      // tourner dans le sens horaire (clockwise) pour remettre
                      // le moteur à sa position de départ
  } else {
    clockwise = false; // si on a effectué moins de 50% de la rotation du moteur
                       // (comparé à la position de départ) on va tourner dans
                       // le sens antihoraire pour remettre le moteur à sa
                       // position de départ plus rapidement
  }

  step(clockwise);
  return true;
}