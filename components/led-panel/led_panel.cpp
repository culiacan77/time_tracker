// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "led_panel.hpp"
#include "driver/gpio.h"
#include "esp_log.h"

// du à mon cablage hardware je dois corriger l'orientation entre la grille
// software et la grille hardware. ce n'est pas exactement une rotation de 90
// degrés anti horaire

// table de correction gamma pour compenser la non linéarité de la perception
// visuelle
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

static constexpr int kBrightnessMax_ =
    100; // Valeur maximale de luminosité, low brightness limit available tint

// liste position des leds à allumer selon le pattern
// mode pause
static const std::vector<std::vector<int>> kLightPatternStop = {
    {0, 0}, {1, 0}, {2, 0}, {0, 2}, {1, 2}, {2, 2}};
// mode normal + rainbow (sans fade pr full lit)
static const std::vector<std::vector<int>> kLightPatternRun = {
    {0, 0}, {0, 1}, {0, 2}, {1, 2}, {1, 1}, {1, 0}, {2, 0}, {2, 1}, {2, 2}};
// mode reset
static const std::vector<std::vector<int>> kLightPatternReset = {
    {0, 1}, {0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}};

static const std::vector<std::vector<int>> kFullLit = {
    {0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}};

static const char *kTag = "led_panel";

const int HueRange = 360;

// constructeur
LedPanel::LedPanel(gpio_num_t switch_up, gpio_num_t switch_down,
                   gpio_num_t gpioPin, led_strip_handle_t led_strip_handle)
    : switch_up_(switch_up), switch_down_(switch_down),
      led_strip_handle_(led_strip_handle) {
  LightingPattern_ = kLightPatternRun;
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << switch_up_) | (1ULL << switch_down_);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK(gpio_config(&io_conf));

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

void LedPanel::update() {
  int switch_up_state = gpio_get_level(switch_up_);
  int switch_down_state = gpio_get_level(switch_down_);

  // mode : 3 = switch neutral position (run normal)
  //        1 = switch left position (stop clockwheel)
  //        2 = switch right position (change led color)
  int mode = switch_up_state << 1 | switch_down_state;

  switch (mode) {
  case 3: // normal operation
    this->setAnimationPattern(kLightPatternRun);

    break;
  case 1: // switch left position (stop clockwheel)
    this->setAnimationPattern(kLightPatternStop);

    break;
  case 2: // switch right position (change led color)
    this->setAnimationPattern(kFullLit);
    this->shiftLedColor();

    break;
  }
};
// problème: light trail devra etre défini de par l'exterieur.
// faire en sorte d'avoir une fonction qui parcour le tableau et lui donner un
// pointeur vers une fonction dans sa définition. lors de son appel on pourra
// donner la fonction dim / clear our lit comme argument.