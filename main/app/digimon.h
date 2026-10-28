#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "digimon_types.h"

// Nucleo de la mascota virtual. Capa de datos pura: no toca pantalla, GPIO ni
// drivers. La forma, atributo, evolucion y estadisticas salen de digimon_cfg.

enum DgAct : uint8_t {
  DG_ACT_IDLE = 0,
  DG_ACT_HAPPY,
  DG_ACT_TRAIN,
  DG_ACT_ATTACK,
  DG_ACT_HURT,
};

// La forma 0 del config es el huevo; a los 3 minutos eclosiona a 黑球兽.
static constexpr uint8_t EGG_FORM = 0;
static constexpr uint8_t EGG_HATCH_MIN = 3;

class Digimon {
 public:
  uint8_t form = 0;             // especie actual (indice en g_sp[])
  uint32_t ageMin = 0;          // minutos de juego
  uint32_t exp = 0;             // experiencia (tiempo / entrenamiento / combate)
  uint16_t hp = 0;              // HP actual
  uint16_t sp = 0;              // SP actual
  uint16_t weightBonus10 = 0;   // peso extra por entrenamiento (0.1 kg)
  uint16_t train[DG_STAT_N] = {};
  uint16_t wins = 0;            // victorias RPS
  uint8_t oneShot = 255;        // DgAct mostrada de golpe (255 = ninguna)
  uint32_t oneShotUntil = 0;
  bool dirty = false;
  bool justHatched = false;     // evento para la UI

  void begin();
  bool update(uint32_t nowMs);   // regeneracion + exp por tiempo
  void sanitize();               // limita valores importados por la web
  void save();
  void load();
  void reset();

  uint16_t level() const { return (uint16_t)(1 + exp / 100); }
  uint16_t statMax(uint8_t i) const;
  uint16_t maxHp() const { return statMax(DG_STAT_HP); }
  uint16_t maxSp() const { return statMax(DG_STAT_SP); }
  uint16_t atk() const { return statMax(DG_STAT_ATK); }
  uint16_t def() const { return statMax(DG_STAT_DEF); }
  uint16_t intel() const { return statMax(DG_STAT_INT); }
  uint16_t spi() const { return statMax(DG_STAT_SPI); }
  uint16_t spd() const { return statMax(DG_STAT_SPD); }
  uint16_t power() const {
    return (uint16_t)(hp + sp + atk() * 2 + def() * 2 + intel() + spi() + spd());
  }
  uint16_t weight10() const;
  const char *name() const;
  uint8_t type() const;
  bool isEgg() const { return form == EGG_FORM; }
  uint8_t hatchLeft() const;

  // Evolucion / degradacion seleccionables ahora mismo.
  int evoList(uint8_t *out, int max) const;
  int devoList(uint8_t *out, int max) const;
  void evolveTo(uint8_t target);
  void devolveTo(uint8_t target);

  // Entrenamiento: reparte puntos y sube un poco el peso. Devuelve los puntos.
  uint16_t trainGame(uint16_t hits, char *summary, size_t n);
  void addExp(uint32_t add);
  void heal(uint16_t hpAdd, uint16_t spAdd);
  uint8_t act(uint32_t nowMs) const;

 private:
  uint32_t nextTickAt = 0;
  void tick();
  void clampVitals();
  void defaults();
};
