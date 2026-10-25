#include "battle.h"

#include "digimon_cfg.h"
#include <Arduino.h>
#include <esp_random.h>

#include <string.h>

static void copyStr(char *dst, size_t n, const char *src) {
  if (!src) { dst[0] = 0; return; }
  strncpy(dst, src, n - 1);
  dst[n - 1] = 0;
}

const char *rpsName(uint8_t move) {
  switch (move) {
    case RPS_SCISSORS: return "剪刀";
    case RPS_ROCK:     return "石头";
    case RPS_PAPER:    return "布";
    default:           return "?";
  }
}

static void fillEnemy(BattleSide &s, uint8_t sp, uint16_t level) {
  memset(&s, 0, sizeof(s));
  if (g_spN <= 0) return;
  const CfgSpecies &c = g_sp[sp];
  copyStr(s.name, sizeof(s.name), c.name);
  s.type = c.type;
  s.level = level;
  s.hpMax = (uint16_t)(c.base[DG_STAT_HP] + level * 8);
  s.spMax = (uint16_t)(c.base[DG_STAT_SP] + level * 3);
  s.hp = s.hpMax;
  s.sp = s.spMax;
}

void battleBegin(Battle &b, Digimon &pet) {
  memset(&b, 0, sizeof(b));
  if (g_spN <= 0) { b.state = BS_LOSE; return; }

  copyStr(b.me.name, sizeof(b.me.name), pet.name());
  b.me.type = pet.type();
  b.me.level = pet.level();
  b.me.hpMax = pet.maxHp();
  b.me.spMax = pet.maxSp();
  b.me.hp = pet.hp > b.me.hpMax ? b.me.hpMax : pet.hp;
  b.me.sp = pet.sp > b.me.spMax ? b.me.spMax : pet.sp;

  int maxFoe = 3 + (int)pet.level() / 3;
  if (maxFoe >= g_spN) maxFoe = g_spN - 1;
  if (maxFoe < 0) maxFoe = 0;
  uint8_t foeForm = (uint8_t)(esp_random() % (uint32_t)(maxFoe + 1));
  int lvl = (int)pet.level() + (int)(esp_random() % 3) - 1;
  if (lvl < 1) lvl = 1;
  fillEnemy(b.foe, foeForm, (uint16_t)lvl);

  b.state = BS_ACTIVE;
  b.playerMove = 0xFF;
  b.foeMove = 0xFF;
}

static void winRound(Battle &b, Digimon &pet) {
  b.state = BS_WIN;
  uint32_t gain = 20 + (uint32_t)b.foe.level * 5;
  pet.addExp(gain);

  uint16_t pts = (uint16_t)(2 + b.foe.level / 4);
  if (pts > 8) pts = 8;
  for (uint16_t i = 0; i < pts; i++) {
    int stat = DG_STAT_ATK + (int)(esp_random() % (DG_STAT_SPD - DG_STAT_ATK + 1));
    pet.train[stat] = (uint16_t)(pet.train[stat] + 1);
  }
  if (pet.weightBonus10 > 1) pet.weightBonus10 = (uint16_t)(pet.weightBonus10 - 1);
  pet.oneShot = DG_ACT_HAPPY;
  pet.oneShotUntil = millis() + 2500;
  pet.save();
}

static void loseRound(Battle &b, Digimon &pet) {
  b.state = BS_LOSE;
  uint16_t dmg = (uint16_t)(pet.maxHp() / 10 + 1);
  pet.hp = (pet.hp > dmg) ? (uint16_t)(pet.hp - dmg) : 1;
  pet.sp = (pet.sp > 5) ? (uint16_t)(pet.sp - 5) : 0;
  pet.addExp(5);
  pet.oneShot = DG_ACT_HURT;
  pet.oneShotUntil = millis() + 2500;
  b.me.hp = pet.hp;
  b.me.sp = pet.sp;
  pet.save();
}

void battlePlay(Battle &b, Digimon &pet, uint8_t move) {
  if (b.state != BS_ACTIVE || move > RPS_PAPER) return;

  b.playerMove = move;
  b.foeMove = (uint8_t)(esp_random() % 3);

  int diff = ((int)b.playerMove - (int)b.foeMove + 3) % 3;
  if (diff == 0) {
    b.state = BS_DRAW;
    pet.addExp(8);
    pet.save();
  } else if (diff == 1) {
    winRound(b, pet);
  } else {
    loseRound(b, pet);
  }

  b.me.hp = pet.hp;
  b.me.sp = pet.sp;
}
