#include "digimon.h"

#include "digimon_cfg.h"
#include <Arduino.h>
#include <Preferences.h>

#include <stdio.h>
#include <string.h>

static const uint32_t DG_TICK_MS = 60000UL;
static const uint32_t EXP_PER_TICK = 2;
static const uint8_t DG_SAVE_VER = 6;
static const uint8_t GROWTH[DG_STAT_N] = { 8, 3, 2, 2, 2, 2, 2 };

static Preferences s_prefs;

void Digimon::defaults() {
  form = 0;
  ageMin = 0;
  exp = 0;
  weightBonus10 = 0;
  memset(train, 0, sizeof(train));
  wins = 0;
  hp = maxHp();
  sp = maxSp();
  oneShot = 255;
  oneShotUntil = 0;
  justHatched = false;
  dirty = true;
}

void Digimon::begin() {
  s_prefs.begin("digimon", false);
  load();
  clampVitals();
  if (hp == 0) hp = maxHp();
  if (sp == 0 && maxSp() > 0) sp = maxSp();
  nextTickAt = millis() + DG_TICK_MS;
}

const char *Digimon::name() const {
  if (g_spN <= 0 || form >= (uint8_t)g_spN) return "数码蛋";
  return g_sp[form].name;
}

uint8_t Digimon::type() const {
  if (g_spN <= 0 || form >= (uint8_t)g_spN) return DG_TYPE_FREE;
  return g_sp[form].type;
}

uint16_t Digimon::statMax(uint8_t i) const {
  if (i >= DG_STAT_N || g_spN <= 0 || form >= (uint8_t)g_spN) return 1;
  int v = (int)g_sp[form].base[i] + (int)level() * GROWTH[i] + (int)train[i];
  return (uint16_t)(v < 1 ? 1 : v);
}

uint16_t Digimon::weight10() const {
  if (g_spN <= 0 || form >= (uint8_t)g_spN) return 0;
  return (uint16_t)(g_sp[form].weight10 + weightBonus10);
}

int Digimon::evoList(uint8_t *out, int max) const {
  int n = 0;
  for (int i = 0; i < g_evoN; i++) {
    const CfgEvo &e = g_evo[i];
    if (e.from != form) continue;
    if (ageMin < (uint32_t)(e.age > 0 ? e.age : 0)) continue;
    if (e.lvl && (int32_t)level() < e.lvl) continue;
    if (e.hp    && (int32_t)maxHp() < e.hp)    continue;
    if (e.atk   && (int32_t)atk()   < e.atk)   continue;
    if (e.def   && (int32_t)def()   < e.def)   continue;
    if (e.intel && (int32_t)intel() < e.intel) continue;
    if (e.spi   && (int32_t)spi()   < e.spi)   continue;
    if (e.spd   && (int32_t)spd()   < e.spd)   continue;
    if (e.wmin  && (int32_t)weight10() < e.wmin * 10) continue;
    if (e.wmax  && (int32_t)weight10() > e.wmax * 10) continue;
    if (e.wins  && (int32_t)wins < e.wins) continue;
    if (e.to >= (uint8_t)g_spN) continue;
    bool dup = false;
    for (int k = 0; k < n; k++) if (out[k] == e.to) dup = true;
    if (!dup && n < max) out[n++] = e.to;
  }
  return n;
}

int Digimon::devoList(uint8_t *out, int max) const {
  int n = 0;
  for (int i = 0; i < g_evoN; i++) {
    const CfgEvo &e = g_evo[i];
    if (e.to != form) continue;
    if (e.from >= (uint8_t)g_spN) continue;
    bool dup = false;
    for (int k = 0; k < n; k++) if (out[k] == e.from) dup = true;
    if (!dup && n < max) out[n++] = e.from;
  }
  return n;
}

void Digimon::evolveTo(uint8_t target) {
  if (target >= (uint8_t)g_spN) return;
  bool valid = false;
  for (int i = 0; i < g_evoN; i++) {
    if (g_evo[i].from == form && g_evo[i].to == target) { valid = true; break; }
  }
  if (!valid) return;
  form = target;
  heal(maxHp(), maxSp());
  oneShot = DG_ACT_HAPPY;
  oneShotUntil = millis() + 2200;
  dirty = true;
}

void Digimon::devolveTo(uint8_t target) {
  if (target >= (uint8_t)g_spN) return;
  bool valid = false;
  for (int i = 0; i < g_evoN; i++) {
    if (g_evo[i].to == form && g_evo[i].from == target) { valid = true; break; }
  }
  if (!valid) return;
  form = target;
  weightBonus10 = (uint16_t)(weightBonus10 * 3 / 4);
  exp = exp * 3 / 4;
  heal(maxHp(), maxSp());
  oneShot = DG_ACT_HAPPY;
  oneShotUntil = millis() + 2200;
  dirty = true;
}

uint16_t Digimon::trainGame(uint16_t hits, char *summary, size_t n) {
  uint16_t points = (uint16_t)(1 + hits / 3);
  if (points > 12) points = 12;

  for (uint16_t k = 0; k < points; k++) {
    int stat = DG_STAT_ATK + (int)(esp_random() % (DG_STAT_SPD - DG_STAT_ATK + 1));
    train[stat] = (uint16_t)(train[stat] + 1);
  }
  train[DG_STAT_HP] = (uint16_t)(train[DG_STAT_HP] + 1);
  train[DG_STAT_SP] = (uint16_t)(train[DG_STAT_SP] + 1);

  uint16_t wgain = (uint16_t)(2 + points / 3);
  weightBonus10 = (uint16_t)(weightBonus10 + wgain);
  exp += 10 + hits;
  heal(maxHp(), maxSp());
  oneShot = DG_ACT_TRAIN;
  oneShotUntil = millis() + 2200;
  dirty = true;

  if (summary && n) {
    snprintf(summary, n, "训练完成 能力点+%u 体重+%u.%ukg",
             (unsigned)points, (unsigned)(wgain / 10), (unsigned)(wgain % 10));
  }
  return points;
}

void Digimon::addExp(uint32_t add) {
  exp += add;
  dirty = true;
}

void Digimon::heal(uint16_t hpAdd, uint16_t spAdd) {
  uint32_t nh = (uint32_t)hp + hpAdd;
  uint32_t ns = (uint32_t)sp + spAdd;
  hp = (uint16_t)(nh > maxHp() ? maxHp() : nh);
  sp = (uint16_t)(ns > maxSp() ? maxSp() : ns);
  dirty = true;
}

void Digimon::clampVitals() {
  if (hp > maxHp()) hp = maxHp();
  if (sp > maxSp()) sp = maxSp();
}

void Digimon::tick() {
  ageMin++;

  if (isEgg()) {
    if (g_spN > 1 && ageMin >= EGG_HATCH_MIN) {
      form = 1;  // 黑球兽
      heal(maxHp(), maxSp());
      oneShot = DG_ACT_HAPPY;
      oneShotUntil = millis() + 2500;
      justHatched = true;
      dirty = true;
    }
    return;
  }

  exp += EXP_PER_TICK;

  uint16_t hpmax = maxHp(), spmax = maxSp();
  if (hp < hpmax) {
    uint32_t v = (uint32_t)hp + hpmax / 20 + 1;
    hp = (uint16_t)(v > hpmax ? hpmax : v);
  }
  if (sp < spmax) {
    uint32_t v = (uint32_t)sp + spmax / 10 + 1;
    sp = (uint16_t)(v > spmax ? spmax : v);
  }
  dirty = true;
}

bool Digimon::update(uint32_t nowMs) {
  bool changed = false;
  if ((int32_t)(nowMs - nextTickAt) >= 0) {
    nextTickAt += DG_TICK_MS;
    tick();
    changed = true;
  }
  if (oneShot != 255 && (int32_t)(nowMs - oneShotUntil) >= 0) {
    oneShot = 255;
    changed = true;
  }
  return changed;
}

uint8_t Digimon::act(uint32_t nowMs) const {
  if (oneShot != 255 && (int32_t)(nowMs - oneShotUntil) < 0) return oneShot;
  return DG_ACT_IDLE;
}

uint8_t Digimon::hatchLeft() const {
  if (!isEgg()) return 0;
  int left = (int)EGG_HATCH_MIN - (int)ageMin;
  return left > 0 ? (uint8_t)left : 0;
}

void Digimon::sanitize() {
  if (g_spN <= 0) form = 0;
  else if (form >= (uint8_t)g_spN) form = (uint8_t)(g_spN - 1);
  if (ageMin > 10000000UL) ageMin = 10000000UL;
  if (exp > 10000000UL) exp = 10000000UL;
  if (weightBonus10 > 50000) weightBonus10 = 50000;
  for (int i = 0; i < DG_STAT_N; i++) {
    if (train[i] > 9999) train[i] = 9999;
  }
  if (wins > 9999) wins = 9999;
  if (hp == 0) hp = maxHp();
  clampVitals();
  dirty = true;
}

void Digimon::save() {
  s_prefs.putUChar("ver", DG_SAVE_VER);
  s_prefs.putUChar("form", form);
  s_prefs.putUInt("age", ageMin);
  s_prefs.putUInt("exp", exp);
  s_prefs.putUShort("wins", wins);
  s_prefs.putUShort("hp", hp);
  s_prefs.putUShort("sp", sp);
  s_prefs.putUShort("wb", weightBonus10);
  s_prefs.putBytes("trn", train, sizeof(train));
  s_prefs.commit();
  dirty = false;
}

void Digimon::load() {
  if (s_prefs.getUChar("ver", 0) != DG_SAVE_VER) {
    defaults();
    save();
    return;
  }

  form = s_prefs.getUChar("form", 0);
  if (g_spN <= 0 || form >= (uint8_t)g_spN) form = 0;
  ageMin = s_prefs.getUInt("age", 0);
  exp = s_prefs.getUInt("exp", 0);
  wins = s_prefs.getUShort("wins", 0);
  weightBonus10 = s_prefs.getUShort("wb", 0);

  size_t got = s_prefs.getBytes("trn", train, sizeof(train));
  if (got != sizeof(train)) memset(train, 0, sizeof(train));

  hp = s_prefs.getUShort("hp", maxHp());
  sp = s_prefs.getUShort("sp", maxSp());
  clampVitals();
  dirty = false;
}

void Digimon::reset() {
  s_prefs.clear();
  defaults();
  save();
}
