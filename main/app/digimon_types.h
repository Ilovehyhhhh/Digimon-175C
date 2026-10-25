#pragma once

#include <stdint.h>
#include <string.h>

// Atributos de数码宝贝 y matriz de eficacia.
// Triangulo clasico: 疫苗 > 病毒 > 数据 > 疫苗.
// 自由 (Free) y 未知 (Unknown) son atributos raros y no entran en el triangulo.

enum DgType : uint8_t {
  DG_TYPE_VACCINE = 0,   // 疫苗
  DG_TYPE_DATA,          // 数据
  DG_TYPE_VIRUS,         // 病毒
  DG_TYPE_FREE,          // 自由
  DG_TYPE_UNKNOWN,       // 未知
  DG_TYPE_COUNT
};

enum DgStat : uint8_t {
  DG_STAT_HP = 0,
  DG_STAT_SP,
  DG_STAT_ATK,
  DG_STAT_DEF,
  DG_STAT_INT,
  DG_STAT_SPI,
  DG_STAT_SPD,
  DG_STAT_N
};

inline const char *dgTypeName(uint8_t t) {
  switch (t) {
    case DG_TYPE_VACCINE: return "疫苗";
    case DG_TYPE_DATA:    return "数据";
    case DG_TYPE_VIRUS:   return "病毒";
    case DG_TYPE_FREE:    return "自由";
    case DG_TYPE_UNKNOWN: return "未知";
    default:              return "?";
  }
}

inline uint8_t dgTypeFromName(const char *s) {
  if (!s) return DG_TYPE_FREE;
  if (strcmp(s, "疫苗") == 0) return DG_TYPE_VACCINE;
  if (strcmp(s, "数据") == 0) return DG_TYPE_DATA;
  if (strcmp(s, "病毒") == 0) return DG_TYPE_VIRUS;
  if (strcmp(s, "未知") == 0) return DG_TYPE_UNKNOWN;
  return DG_TYPE_FREE;
}

// 150 = ventaja, 50 = desventaja, 100 = neutro.
inline uint16_t dgTypeMultiplier(uint8_t atk, uint8_t def) {
  if (atk >= DG_TYPE_COUNT || def >= DG_TYPE_COUNT) return 100;
  if (atk == DG_TYPE_FREE || atk == DG_TYPE_UNKNOWN ||
      def == DG_TYPE_FREE || def == DG_TYPE_UNKNOWN) return 100;
  if ((atk == DG_TYPE_VACCINE && def == DG_TYPE_VIRUS) ||
      (atk == DG_TYPE_VIRUS   && def == DG_TYPE_DATA)  ||
      (atk == DG_TYPE_DATA    && def == DG_TYPE_VACCINE)) return 150;
  if ((atk == DG_TYPE_VIRUS   && def == DG_TYPE_VACCINE) ||
      (atk == DG_TYPE_DATA    && def == DG_TYPE_VIRUS)   ||
      (atk == DG_TYPE_VACCINE && def == DG_TYPE_DATA)) return 50;
  return 100;
}

inline const char *dgStatName(uint8_t i) {
  switch (i) {
    case DG_STAT_HP:  return "体力";
    case DG_STAT_SP:  return "技能";
    case DG_STAT_ATK: return "攻击";
    case DG_STAT_DEF: return "防御";
    case DG_STAT_INT: return "智力";
    case DG_STAT_SPI: return "精神";
    case DG_STAT_SPD: return "速度";
    default:          return "?";
  }
}
