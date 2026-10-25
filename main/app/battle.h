#pragma once

#include <stdint.h>

#include "digimon.h"

// 对战暂时是剪刀石头布：
//   攻击 = 剪刀, 防御 = 石头, 技能 = 布
enum RpsMove : uint8_t {
  RPS_SCISSORS = 0,
  RPS_ROCK,
  RPS_PAPER,
};

enum BattleState : uint8_t {
  BS_ACTIVE = 0,
  BS_WIN,
  BS_LOSE,
  BS_DRAW,
};

struct BattleSide {
  char name[20];
  uint8_t type;
  uint16_t level;
  uint16_t hp, hpMax;
  uint16_t sp, spMax;
};

struct Battle {
  BattleSide me;
  BattleSide foe;
  uint8_t state;
  uint8_t playerMove;
  uint8_t foeMove;
};

void battleBegin(Battle &b, Digimon &pet);
void battlePlay(Battle &b, Digimon &pet, uint8_t move);
const char *rpsName(uint8_t move);
