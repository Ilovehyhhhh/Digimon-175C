#pragma once

#include "lvgl.h"

#include <stdint.h>

// Avatar de la mascota: dibujo procedural de lineas/pixeles, sin texto ni cara.
// Etapas: 0 huevo, 1 baby, 2 rookie, 3 champion, 4 ultimate, 5 mega.
//
// Si hay sprites, basta con apuntar g_petImage a un lv_image_dsc_t: sustituye
// el dibujo procedural sin tocar la logica del juego.
extern const lv_image_dsc_t *g_petImage;

struct PetAvatar {
  lv_obj_t *root = nullptr;
  lv_obj_t *image = nullptr;
  lv_image_dsc_t dsc = {};
  uint32_t *buf = nullptr;
  int size = 0;
  int baseY = 0;
  uint8_t stage = 0xFF;
  uint32_t accent = 0;
  bool realImage = false;
};

void petAvatarCreate(PetAvatar &a, lv_obj_t *parent, int size, int yOffset);
void petAvatarSetSpecies(PetAvatar &a, uint32_t accent, uint8_t stage);
void petAvatarSetMood(PetAvatar &a, int mood, bool excited, bool sleeping);
void petAvatarBob(PetAvatar &a, int dy);
void petAvatarSetImage(PetAvatar &a, const lv_image_dsc_t *dsc);
