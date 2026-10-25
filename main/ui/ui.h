#pragma once

#include "app/digimon.h"

// Crea las pantallas LVGL y arranca los timers del juego.
// Llamar despues de bsp_display_start(), con el display ya disponible.
void ui_init(Digimon &pet);
