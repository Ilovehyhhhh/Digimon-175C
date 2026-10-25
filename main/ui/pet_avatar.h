#pragma once

#include "lvgl.h"

#include <stdint.h>

// Avatar de la mascota aislado de la logica de juego.
//
// Ahora mismo se dibuja con un caracter chino grande. Cuando haya sprites,
// basta con llamar a petAvatarSetImage() con un lv_image_dsc_t: la imagen
// sustituye al texto y a la cara sin tocar el resto de la UI.
// Poner aqui una imagen (lv_image_dsc_t) para sustituir el avatar de texto.
// Dejarlo a nullptr para usar el caracter chino.
extern const lv_image_dsc_t *g_petImage;

struct PetAvatar {
  lv_obj_t *root = nullptr;
  lv_obj_t *image = nullptr;
  lv_obj_t *glyph = nullptr;
  lv_obj_t *eyeL = nullptr;
  lv_obj_t *eyeR = nullptr;
  lv_obj_t *mouth = nullptr;
  int size = 0;
  int baseY = 0;
  int eyeH = 0;
};

void petAvatarCreate(PetAvatar &a, lv_obj_t *parent, int size, int yOffset);
void petAvatarSetSpecies(PetAvatar &a, const char *name, uint32_t accent);
void petAvatarSetMood(PetAvatar &a, int mood, bool excited);
void petAvatarSetBlink(PetAvatar &a, bool blink);
void petAvatarBob(PetAvatar &a, int dy);
void petAvatarSetImage(PetAvatar &a, const lv_image_dsc_t *dsc);
