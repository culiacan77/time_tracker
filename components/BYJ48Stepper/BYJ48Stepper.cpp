#include "BYJ48Stepper.hpp"

#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "inttypes.h"

// uint8_t = unsign int of 8 bits. crochet sans chiffre = la taille du
// tableau sera déterminé par le nombre d'éléments
// partagé par toutes les instances de cette classes et les classes
// dérivées (de cette classe) si il y en avaient
const uint8_t sequence_array[] = {0b1010, 0b0110, 0b0101, 0b1001};

// initialisation d'un tablea de 4 entrée, contenant les 4
// positions de la
// séquence de rotation du moteur. 0b est une convention pour indiquer aux
// humains qu'il s'agit d'une notation binaire. On aurait aussi pu mettre 10,
// 6, 5 et 9, qui sont stocké dans le programme en binaire et correspondent à
// la séquence 1010, 0110, 0101, 1001. la séquence 1010 va être utilisé pour
// mettre 1 sur la 1ère pin, 0 sur la 2ème, 1 sur la 3ème et 0 sur la 4ème
// définition du constructeur de la classe Stepper

const size_t sequence_length =
    sizeof(sequence_array) / sizeof(sequence_array[0]);
// taille du tableau / taille d'un élément du
// tableau = nbr d'élément dans le tableau.

Stepper::Stepper(int steps_per_rotation)
    : steps_per_rotation_(steps_per_rotation), // liste d'initialisation
      current_step_(0) {};

FourPinStepper::FourPinStepper(int steps_per_rotation, gpio_num_t motor_pin_0,
                               gpio_num_t motor_pin_1, gpio_num_t motor_pin_2,
                               gpio_num_t motor_pin_3)
    : Stepper(steps_per_rotation) {

  // configuration des GPIO
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
      (1ULL << motor_pin_0) |
      (1ULL << motor_pin_1) | // le | va combiner les masques, 2 0 reste 0. 0 et
                              // 1 deveiennent 1
      (1ULL << motor_pin_2) |
      (1ULL << motor_pin_3); // le | est un "ou" qui rajoute les bits
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  ESP_ERROR_CHECK(gpio_config(&io_conf)); // arrête le programme si retourne un
                                          // message d'erreur autre que ESP-OK

  // gpio_set_level retourne un message d'erreur pour
  // dire soit que tout est Ok, ou
  // qu'il y a une erreur. ESP_ERROR_CHECK permet de
  // passer à la suite si le message est ok

  motor_pins_[0] = motor_pin_0;
  motor_pins_[1] = motor_pin_1;
  motor_pins_[2] = motor_pin_2;
  motor_pins_[3] = motor_pin_3;
}
// initialisation de l'attribut current_step_ à 0 mieux vaut faire ça qu'une
// attribution dans le constructeur (currentStep_ = 0) c'est une bonne
// habitude à prendre car si on avait un const on ne pourrait pas se permette
// que le constructeur lui donne une valeur random lors de sa création (c'est
// ce que fait le constructeur avant la liste d'initialisation) puis lui donne
// la valeur 0 (un const ne peut pas changer) problème similaire avec les
// pointeurs.

// définition de la méthode step
void FourPinStepper::Step(bool clockwise) {
  // Mise à jour de currentStep selon le sens de rotation
  if (clockwise) {
    // mise à jour de current_step_ par increment de 1
    // s'assurer que current_step_ reste entre 0 et steps_per_rotation_ -1
    current_step_ = (current_step_ + 1) % steps_per_rotation_;
  } else {
    current_step_ =
        (current_step_ + steps_per_rotation_ - 1) % steps_per_rotation_;
  }

  // Calcule la position dans la séquence. le fait de faire %4 +4 permet de
  // ne pas avoir de résultat négatif, tout en gardant le même résultat du
  // modulo. On aurait pu se passer du double %4 si on avait une structure
  // avec current_step_ = current_step_ +4 %4, ce qui nous aurait garanti
  // que current_step_reste compris entre 0 et 4. Mais comme on a besoin que
  // current step_ puisse devenir plus grand pour pouvoir être comparé à
  // steps_per_rotation dans la méthode ResetStep, current_step_ n'a pas de
  // limite et pourrait atteindre n'importe quelle valeur négative si
  // l'utilisateur utilise les boutons pour faire tourner un moteur en sens
  // antihoraire.
  int sequence_array_index_ = current_step_ % sequence_length;

  // commentaires détaillent le cas i = 0
  for (int i = 0; i < number_of_pin; i++) {
    ESP_ERROR_CHECK(gpio_set_level(motor_pins_[i], 0));

    // extinction de tous les moteurs.
    // gpio_set_level retourne un message d'erreur pour
    // dire soit que tout est Ok, ou
    // qu'il y a une erreur. ESP_ERROR_CHECK permet de
    // passer à la suite si le message est ok
  }

  int current_sequence = sequence_array[sequence_array_index_];

  for (int i = 0; i < number_of_pin; i++) {
    if (current_sequence & (1 << i)) {
      // on récupère l'entrée du tableau selon current_sequence, ex la 1ère
      // séquence. on utilise la logique AND grâce à &. On utilise le masque
      // binaire 0001 grâce à 1. à ce stade on décalé de 0 vers la gauche donc
      // 0001. à la prochaine étape il vaudra 0010.
      gpio_set_level(motor_pins_[i], 1);
    }
  }
  // debug
  printf("////////////////////////////////////////////////// \n");
  printf("current_step_ value is: %d \n", current_step_);
  printf("sequence_array_index_ is: %d \n", sequence_array_index_);
  printf("current_sequence value is: %d \n",
         sequence_array[sequence_array_index_]);
}

bool FourPinStepper::ResetStep() {
  int exceding_steps = current_step_ % steps_per_rotation_;
  if (exceding_steps == 0) {
    return false; // return met fin à la méthode ResetStep. le fait que cette
                  // méthode soit un bool fait qu'on doit retourner une valeur
                  // soit true soit false. ça permet de savoir si la méthode est
                  // dans un cas où exceding_steps = 0 où pas.
  }

  // if (exceding_steps >= steps_per_rotation_ / 2) {
  //   clockwise = true; // si on a effectué plus de 50% de la rotation du
  //   moteur
  //                     // (comparé à la position de départ) on va continuer à
  //                     // tourner dans le sens horaire (clockwise) pour
  //                     remettre
  //                     // le moteur à sa position de départ
  // } else {
  //   clockwise = false; // si on a effectué moins de 50% de la rotation du
  //   moteur
  //                      // (comparé à la position de départ) on va tourner
  //                      dans
  //                      // le sens antihoraire pour remettre le moteur à sa
  //                      // position de départ plus rapidement
  // }

  Step(exceding_steps >= steps_per_rotation_ / 2);
  return true;

  // debug
  printf("exceding_steps is: %d \n", exceding_steps);
}