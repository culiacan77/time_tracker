// SPDX-FileCopyrightText: 2026 Thomas Fessler
//
// SPDX-License-Identifier: MIT

#include "led_panel.hpp"
#include "buttonpanel.hpp"
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
    170; // Valeur maximale de luminosité, low brightness limit available tint
static constexpr int kBrightnessLowTreshold_ = 25;
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
LedPanel::LedPanel(ButtonPanel &buttonPanelReference, gpio_num_t gpioPin,
                   led_strip_handle_t led_strip_handle)
    : buttonPanelReference_(buttonPanelReference),
      led_strip_handle_(led_strip_handle) {
  LightingPattern_ = kLightPatternRun;

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
    if (trailLength > ledPanelMatrix_.size()) {
      trailLength = ledPanelMatrix_.size();
    }
  }
  // vide le vecteur BrightnessLUT_
  BrightnessLUT_.clear();
  // remplit le vecteur BrightnessLUT_
  for (int i = 0; i < trailLength; i++) {
    // recuperer la valeur du tableau gamma à la position de brightnessInterval
    // * (i + 1)
    int brightness =
        i * (kBrightnessMax_ - kBrightnessLowTreshold_) / (trailLength - 1) +
        kBrightnessLowTreshold_;
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

void LedPanel::updateMatrix() {
  int n = ledPanelMatrix_.size();

  if (shouldDimLight_) {
    // --- LOGIQUE ANIMATION (TRAINÉE) ---
    int rowIndex = LightingPatternIndex_ % LightingPattern_.size();
    int rowValue = LightingPattern_[rowIndex][0];
    int columnValue = LightingPattern_[rowIndex][1];

    // On place le nouveau point à l'index de luminosité max
    ledPanelMatrix_[rowValue][columnValue] = BrightnessLUT_.size() - 1;

    // On diminue l'intensité des autres points
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        if (ledPanelMatrix_[i][j] > 0) {
          ledPanelMatrix_[i][j]--;
        }
      }
    }
    LightingPatternIndex_++;
  } else {
    // --- LOGIQUE STATIQUE (PAUSE / COULEUR) ---
    // 1. On éteint tout d'abord
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        ledPanelMatrix_[i][j] = 0;
      }
    }

    // 2. On allume UNIQUEMENT les points du pattern actuel
    for (const auto &coord : LightingPattern_) {
      int r = coord[0];
      int c = coord[1];
      // On utilise l'index max du LUT pour que ce soit bien brillant
      ledPanelMatrix_[r][c] = BrightnessLUT_.size() - 1;
    }
  }
}

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

  if (buttonPanelReference_.shouldPause()) {
    shouldDimLight_ = false;
    this->setAnimationPattern(kLightPatternStop);
    // this->clearLedPanelMatrix();
  } else if (buttonPanelReference_.shouldChangeColor()) {
    shouldDimLight_ = false;
    this->setAnimationPattern(kFullLit);
    this->shiftLedColor();
  } else if (buttonPanelReference_.shouldReset()) {
    shouldDimLight_ = true;
    this->setAnimationPattern(kLightPatternReset);
  } else {
    shouldDimLight_ = true;
    this->setAnimationPattern(kLightPatternRun);
  }
}

// comprendre comment est crée le LUT qui semble t'il rempli la matrix et le
// lien entre lui et le gamma. bref comment on détermine l'intensité le fait
// que de base la valeur dans la matrixe est plus grande de 1 et qu'on corrige
// ça lorsque shouldDimLight_ true va poser problème dans le cas ou les led
// doivent rester allumées.
// -> amélioration système:
