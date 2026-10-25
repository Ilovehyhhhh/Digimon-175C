#pragma once

#include "lvgl.h"

#include <stdint.h>

// Avatar procedural: cada especie tiene cuerpo, rasgos y escala propios.
// Sin texto y sin sprites: solo lineas/pixeles + cara con expresion.
//
// Si hay sprites, apuntar g_petImage a un lv_image_dsc_t y se usa en su lugar.
extern const lv_image_dsc_t *g_petImage;

struct PetAvatar {
  lv_obj_t *root = nullptr;
  lv_obj_t *image = nullptr;
  lv_image_dsc_t dsc = {};
  uint32_t *buf = nullptr;
  int size = 0;
  int baseY = 0;
  int scalePct = 100;
  uint8_t form = 0xFF;
  uint8_t expr = 0;
  uint32_t accent = 0;
  bool realImage = false;
};

void petAvatarCreate(PetAvatar &a, lv_obj_t *parent, int size, int yOffset);
void petAvatarSetSpecies(PetAvatar &a, uint32_t accent, uint8_t form);
void petAvatarSetMood(PetAvatar &a, int mood, bool excited, bool sleeping);
void petAvatarBob(PetAvatar &a, int dy);
void petAvatarSetImage(PetAvatar &a, const lv_image_dsc_t *dsc);
