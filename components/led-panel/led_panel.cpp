// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "led_panel.hpp"
#include "driver/gpio.h"
#include "esp_log.h"

// du à mon cablage hardware je dois corriger l'orientation entre la grille
// software et la grille hardware. ce n'est pas exactement une rotation de 90
// degrés anti horaire

static constexpr int kLedPanelLeds =
    9; // Définissez le nombre total de LEDs dans le panneau

static const int kGammaCorrection[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
    2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,
    8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,
    14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,
    21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,
    31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,
    43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,
    58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,
    75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,
    96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119,
    120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146,
    148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177,
    180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252,
    255};

static constexpr int kBrightnessMax_ = 30; // Valeur maximale de luminosité

static const char *kTag = "led_panel";

const int HueRange = 360;

// constructeur
LedPanel::LedPanel(gpio_num_t gpioPin, led_strip_rmt_config_t *rmt_config) {
  // --- Initialisation du Panneau LED ---
  led_strip_config_t panel_strip_config = {};
  panel_strip_config.strip_gpio_num = gpioPin;
  panel_strip_config.max_leds = kLedPanelLeds; // Utilisez le bon nombre de LEDs
  panel_strip_config.led_model = LED_MODEL_WS2812; // Assurez-vous du bon modèle
  panel_strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB;

  ESP_ERROR_CHECK(led_strip_new_rmt_device(&panel_strip_config, rmt_config,
                                           &led_strip_handle_));

  // pas besoin d'initialiser un autre rmt_config, il va pour les deux led strip
  ESP_LOGI(kTag, "Created Panel LED strip object");
};

void LedPanel::setAnimationPattern(
    const std::vector<std::vector<int>> &currentLightPattern) {
  LightingPattern_ = currentLightPattern; // le pointeur LightingPattern stock
                                          // l'adresse de currentLightPattern
};

// integrer gamma correction
void LedPanel::setTrailLength(int trailLength) {
  if (trailLength < 2) {
    trailLength = 2;
  }
  // vide le vecteur BrightnessLUT_
  BrightnessLUT_.clear();
  // remplit le vecteur BrightnessLUT_
  for (int i = 0; i < trailLength; i++) {
    // recuperer la valeur du tableau gamma à la position de brightnessInterval
    // * (i + 1)
    int brightness = i * kBrightnessMax_ / (trailLength - 1);
    BrightnessLUT_.push_back(kGammaCorrection[brightness]);
  }
};

void LedPanel::clearLedPanelMatrix() {

  for (auto line : ledPanelMatrix_) {
    for (int i = 0; i < line.size(); ++i) {
      line[i] = 0;
    }
  }
};

void LedPanel::updateMatrix(bool shouldDimLight_) {
  int rowIndex = LightingPatternIndex_ % (LightingPattern_).size();
  int rowValue = (LightingPattern_)[rowIndex][0];
  int columnValue = (LightingPattern_)[rowIndex][1];
  // lightTrail est aussi la valeur de brightness max
  ledPanelMatrix_[rowValue][columnValue] = BrightnessLUT_.size();
  // La valeur qu'on vient d'assigner dans notre matrix est plus grande de 1
  // que l'index des elements de notre vecteur LUT, mais la suite va
  // soustraire 1 à toutes les valeurs (ce qui gère la baisse d'intensité de
  // la trainée). Donc si on a mit 3 dans le tableau, on se retrouve avec 2,
  // ce qui permet d'accéder au 2ème élément du vecteur
  if (shouldDimLight_) {
    int n = ledPanelMatrix_.size();
    for (int i = 0; i < n; ++i) {   // i est l'indice de ligne (row)
      for (int j = 0; j < n; ++j) { // j est l'indice de colonne (column)

        if (ledPanelMatrix_[i][j] > 0) {
          // décrémente l'index de luminosité
          ledPanelMatrix_[i][j]--;
        }
      }
    }
  }
  LightingPatternIndex_++;
};

void LedPanel::litLedPanel() {

  const uint8_t SATURATION_MAX = 255;
  int n = ledPanelMatrix_.size();
  // Copie les éléments de l'ancienne matrice vers la nouvelle matrice
  for (int i = 0; i < n; ++i) {   // i est l'indice de ligne (row)
    for (int j = 0; j < n; ++j) { // j est l'indice de colonne (column)

      int brightness =
          BrightnessLUT_[ledPanelMatrix_[i][j]]; // Récupère la valeur de
                                                 // luminosités

      ESP_LOGI(kTag, "brightness[%d][%d]=%d", i, j, brightness);

      int ledIndex = j * n + i; // calcule l'index de la led
      led_strip_set_pixel_hsv(led_strip_handle_, ledIndex, Hue_, SATURATION_MAX,
                              brightness);
    }
  }
  // Affiche toutes les LEDs mises à jour en même temps
  led_strip_refresh(led_strip_handle_);
};

void LedPanel::shiftLedColor() {
  Hue_++;           // Increment first
  Hue_ %= HueRange; // Then wrap around
};

// problème: light trail devra etre défini de par l'exterieur.
// faire en sorte d'avoir une fonction qui parcour le tableau et lui donner un
// pointeur vers une fonction dans sa définition. lors de son appel on pourra
// donner la fonction dim / clear our lit comme argument.