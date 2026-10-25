#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "digimon_types.h"

// Configuracion (JSON embebido) -> tablas en RAM. Edita el JSON de
// digimon_cfg.cpp sin tocar el resto del codigo.

struct CfgSpecies {
  char name[20];
  uint8_t type;
  uint16_t base[DG_STAT_N];
  uint16_t weight10;          // peso base en unidades de 0.1 kg
};

struct CfgEvo {
  uint8_t from, to;
  int32_t age;                // minutos
  int32_t lvl;
  int32_t hp, atk, def, intel, spi, spd;
  int32_t wmin, wmax;         // kg
};

#define CFG_MAX_SPECIES 32
#define CFG_MAX_EVOS    64

extern CfgSpecies g_sp[CFG_MAX_SPECIES];  extern int g_spN;
extern CfgEvo     g_evo[CFG_MAX_EVOS];    extern int g_evoN;

void cfgLoad(void);   // parsea el JSON (llamar una vez al arrancar)
