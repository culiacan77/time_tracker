// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "led_panel.hpp"

#include "driver/gpio.h"

constexpr uint32_t kRed = 0x220000;

int kBrightnessLevel = 255;

void led_panel::Update(int64_t current_time) {
  // read the switch states
  int switch_left_state = gpio_get_level(switch_left_);
  int switch_right_state = gpio_get_level(switch_right_);

  // mode : 0 = no switch pressed (run normal)
  //        1 = right switch pressed (change color)
  //        2 = left switch pressed (pause)
  int mode = switch_left_state << 1 | switch_right_state;

  int64_t elapsed_time = current_time - time_origin_;

  // Imaginons que tu calcules le temps écoulé depuis la dernière frame
  double delta_time = elapsed_time - last_update_time_;
  double update_tracker_ += delta_time;

  if (update_tracker_ >= updateFrequency_) {
    switch (mode) {
    case 0: // normal
            // led_strip_ c'est le led handle
      currentLedPattern_ = KSNAKE;
      brightnessFadingIndex = 50;
      LedTrail_ = 5;
      // brightnessFadingIndex doit etre basé sur le brightness max et le
      // ledtrail
      break;
    case 1: // right switch pressed: change color
      currentLedPattern_ = KSNAKE;
      brightnessFadingIndex = 50;
      LedTrail_ = 5;
      break;
    case 2:
      // left switch pressed: pause
      // charger le pattern de pause
      // assigner ledTrail à la longueur du pattern de pause
      // brightnessFadingIndex = 0
      currentLedPattern_ = KPAUSE;
      brightnessFadingIndex = 0;
      LedTrail_ = getPatternLength(currentLedPattern_, ledPanelIndex_);
      color = kRed;
      break;
    }
    currentPatternLength_ = getPatternLength(currentLedPattern_);
    current_position_ = updatePosition(currentLedPattern_, ledPanelIndex_,
                                       currentPatternLength_);

    for (int i = 0; i < LedTrail_; i++) {
      led_strip_set_pixel(led_strip_,
                          (current_position_ - i) % currentPatternLength_,
                          color, color >> 16 & 0xFF - brightnessFadingIndex * i,
                          color >> 8 & 0xFF -

                                           *i,
                          color & 0xFF - brightnessFadingIndex * i);
    };
    led_strip_refresh(led_strip_);
    ledPanelIndex_++ % currentPatternLength_;

    // En soustrayant updateFrequency_ de update_tracker_, on garde le
    // "surplus" de temps pour la prochaine boucle et on évite d'incrémenter
    // sans fin (pas d'overflow)
    update_tracker_ -= updateFrequency_;
  }
};

// update du led panel selon le case

// Pour connaître la longueur réelle
// (compte les chiffres hexadécimaux
// non nuls)

int updatePosition(uint64_t pattern, int ledPanelIndex, int patternLength) {
  return (pattern >> (patternLength - ledPanelIndex - 1) * 4) &
         0xF; // on est en hexadécimal, un digit = 4 bits donc 0xF -> 16 en base
              // 10 -> 1111 en binaire
};

int LedPanel::getPatternLength(uint64_t pattern) {
  int patternLength = 0;
  while (pattern > 0) {
    pattern >> 1; // décale les bits de pattern de 1 vers la droite
    patternLength++;
  }
  return patternLength;
};

int LedPanel::getPatternLength(uint64_t pattern) {
  if (pattern == 0)
    return 0;
  int length = 0;
  while (pattern > 0) {
    pattern >>= 4; // On décale de 4 bits vers la droite, et on update pattern
                   // avec cette nouvelle valeur (fait grâce au =) (dans les
                   // faits ça supprime le chifre le plus à droite)
    length++;
  }
  return length;
};

// on est pas en binaire ou on cherche a lire des 0 et des 1 mais des
// valeures de 0 à 9. On va utiliser l'hexadecimale, soit 2^4 bit pour
// encoder un chiffre (donc valeures de 0 à 15). Si on avait pris 2^3
// on aurait pas pu lire le 9 car valeurs de 0 à 8. comme on a des
// packet de 4 bits alloués à chaque chiffre une fois converti en
// binaire (ce dont a besoin l'ordi) il est nécessaire de corriger la
// position en multiqpliant par 4. 0xF crée un masque de 4 bits, à
// savoir: 1111. on aurait pas pu utiliser octodécimal (2^3) bits pour
// contenir la valeur d'un digit car elle va de 0 à 8. ok ça fait 9
// valeurs mais nous il nous en faut 10, de 0 à 9 c'est ça

//------------------TRASH-----------------

// trop couteux en mémoire d'utiliser log10. Mieux vaut faire du bitshifting.

// int LedPanel::getPatternLength(int n) {
//  return floor(log10(n) + 1);
//};

// log10(x) nous retourne la puissance qu'il faut mettre à 10 pour former le
// nombre x. floors enlève la partie décimale du log10. ça nous dit à quelle
// puissance élever 10 pour trouver x mais n'a pas pris en compte les unités,
// donc on rajoute 1