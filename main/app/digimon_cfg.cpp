#include "digimon_cfg.h"
#include "cJSON.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "cfg";

// Formato de una especie:
//   name     nombre visible (el primer caracter hace de avatar)
//   type    疫苗 / 数据 / 病毒 / 自由 / 未知
//   weight  peso base en kg
//   stats   [HP, SP, ATK, DEF, INT, SPI, SPD]
//
// Ramas de evolucion (inspiradas en Digimon World: Next Order, escaladas a
// este firmware):
//   age  minutos de vida      lvl  nivel minimo      wins victorias RPS
//   hp/atk/def/intel/spi/spd  umbral de estadistica (0 = no se comprueba)
//   wmin/wmax                 rango de peso en kg (0 = sin limite)
static const char CFG_JSON[] = R"JSON(
{
  "species": [
    { "name": "数码蛋",     "type": "未知", "weight": 1,   "stats": [10, 5, 1, 1, 1, 1, 1] },
    { "name": "黑球兽",     "type": "自由", "weight": 5,   "stats": [45, 10, 8, 6, 4, 4, 6] },
    { "name": "滚球兽",     "type": "自由", "weight": 10,  "stats": [70, 18, 16, 12, 8, 8, 14] },
    { "name": "亚古兽",     "type": "病毒", "weight": 35,  "stats": [120, 38, 34, 22, 14, 16, 26] },
    { "name": "加布兽",     "type": "数据", "weight": 40,  "stats": [112, 36, 30, 26, 16, 18, 24] },
    { "name": "巴达兽",     "type": "疫苗", "weight": 14,  "stats": [95, 42, 20, 16, 22, 28, 28] },
    { "name": "比丘兽",     "type": "疫苗", "weight": 18,  "stats": [100, 44, 22, 18, 24, 30, 30] },
    { "name": "甲虫兽",     "type": "疫苗", "weight": 30,  "stats": [110, 40, 24, 26, 28, 22, 24] },
    { "name": "哥玛兽",     "type": "疫苗", "weight": 25,  "stats": [130, 38, 26, 24, 18, 20, 22] },
    { "name": "巴鲁兽",     "type": "数据", "weight": 20,  "stats": [105, 42, 22, 20, 26, 26, 24] },
    { "name": "小狗兽",     "type": "疫苗", "weight": 12,  "stats": [90, 46, 18, 14, 26, 32, 30] },
    { "name": "暴龙兽",     "type": "病毒", "weight": 120, "stats": [230, 62, 60, 44, 24, 28, 40] },
    { "name": "加鲁鲁兽",   "type": "数据", "weight": 95,  "stats": [210, 60, 52, 42, 30, 34, 46] },
    { "name": "天使兽",     "type": "疫苗", "weight": 32,  "stats": [175, 82, 34, 32, 46, 56, 48] },
    { "name": "巴多拉兽",   "type": "疫苗", "weight": 45,  "stats": [185, 80, 40, 34, 40, 50, 50] },
    { "name": "比多兽",     "type": "疫苗", "weight": 110, "stats": [220, 74, 48, 52, 50, 40, 38] },
    { "name": "海狮兽",     "type": "疫苗", "weight": 130, "stats": [240, 70, 54, 50, 30, 36, 36] },
    { "name": "仙人掌兽",   "type": "数据", "weight": 70,  "stats": [200, 76, 44, 44, 52, 48, 40] },
    { "name": "迪路兽",     "type": "疫苗", "weight": 28,  "stats": [165, 86, 40, 30, 50, 60, 54] },
    { "name": "恶魔兽",     "type": "病毒", "weight": 80,  "stats": [220, 70, 80, 50, 40, 50, 60] },
    { "name": "机械暴龙兽", "type": "病毒", "weight": 260, "stats": [390, 92, 94, 78, 42, 54, 62] },
    { "name": "兽人加鲁鲁", "type": "数据", "weight": 185, "stats": [350, 90, 82, 66, 48, 58, 78] },
    { "name": "神圣天使兽", "type": "疫苗", "weight": 75,  "stats": [330, 142, 60, 58, 88, 100, 74] },
    { "name": "伽楼达兽",   "type": "疫苗", "weight": 90,  "stats": [340, 138, 70, 60, 94, 104, 72] },
    { "name": "超比多兽",   "type": "疫苗", "weight": 220, "stats": [400, 120, 80, 100, 96, 80, 70] },
    { "name": "祖顿兽",     "type": "疫苗", "weight": 280, "stats": [430, 110, 100, 96, 60, 70, 64] },
    { "name": "花仙兽",     "type": "数据", "weight": 120, "stats": [320, 140, 62, 62, 100, 110, 84] },
    { "name": "天女兽",     "type": "疫苗", "weight": 55,  "stats": [310, 150, 58, 56, 100, 120, 86] },
    { "name": "丧尸暴龙兽", "type": "未知", "weight": 300, "stats": [430, 72, 152, 62, 20, 32, 52] },
    { "name": "吸血魔兽",   "type": "病毒", "weight": 160, "stats": [380, 110, 110, 90, 90, 110, 100] },
    { "name": "战斗暴龙兽", "type": "病毒", "weight": 400, "stats": [570, 132, 142, 112, 66, 82, 94] },
    { "name": "钢铁加鲁鲁", "type": "数据", "weight": 330, "stats": [530, 130, 126, 120, 80, 94, 106] },
    { "name": "究极天使兽", "type": "疫苗", "weight": 155, "stats": [490, 205, 94, 90, 134, 152, 102] },
    { "name": "凤凰兽",     "type": "疫苗", "weight": 180, "stats": [500, 200, 96, 88, 140, 160, 110] },
    { "name": "力神比多兽", "type": "疫苗", "weight": 380, "stats": [600, 180, 130, 150, 130, 120, 110] },
    { "name": "蛇颈龙兽",   "type": "数据", "weight": 450, "stats": [650, 170, 120, 130, 100, 120, 120] },
    { "name": "蔷薇兽",     "type": "数据", "weight": 220, "stats": [520, 200, 110, 110, 150, 160, 130] },
    { "name": "神圣天女兽", "type": "疫苗", "weight": 120, "stats": [510, 220, 100, 96, 150, 180, 140] },
    { "name": "究极吸血魔兽","type": "病毒", "weight": 340, "stats": [620, 160, 180, 140, 120, 150, 130] },
    { "name": "奥米加兽",   "type": "疫苗", "weight": 500, "stats": [750, 220, 210, 190, 160, 180, 170] },
    { "name": "机械邪龙兽", "type": "病毒", "weight": 420, "stats": [650, 170, 180, 160, 120, 140, 120] }
  ],
  "evolve": [
    { "from": 0,  "to": 1,  "age": 3 },
    { "from": 1,  "to": 2,  "age": 8 },

    { "from": 2,  "to": 3,  "age": 15, "atk": 20, "wmin": 10 },
    { "from": 2,  "to": 4,  "age": 15, "def": 16, "wmin": 10 },
    { "from": 2,  "to": 5,  "age": 18, "spi": 12, "wmax": 11 },
    { "from": 2,  "to": 6,  "age": 18, "intel": 12, "wmax": 11 },
    { "from": 2,  "to": 7,  "age": 18, "intel": 14, "wmax": 12 },
    { "from": 2,  "to": 8,  "age": 18, "hp": 90, "wmin": 12 },
    { "from": 2,  "to": 9,  "age": 18, "intel": 12, "wmax": 10 },
    { "from": 2,  "to": 10, "age": 20, "spi": 14, "wmax": 10 },

    { "from": 3,  "to": 11, "age": 45, "lvl": 2, "atk": 45, "wmin": 15 },
    { "from": 3,  "to": 12, "age": 50, "lvl": 2, "spd": 40, "wmin": 15 },
    { "from": 4,  "to": 12, "age": 45, "lvl": 2, "spd": 40, "wmin": 20 },
    { "from": 4,  "to": 11, "age": 50, "lvl": 2, "atk": 45, "wmin": 20 },
    { "from": 5,  "to": 13, "age": 45, "lvl": 2, "spi": 35, "wmax": 25 },
    { "from": 5,  "to": 19, "age": 45, "lvl": 2, "atk": 35 },
    { "from": 6,  "to": 14, "age": 45, "lvl": 2, "spi": 30, "wmax": 25 },
    { "from": 6,  "to": 13, "age": 50, "lvl": 2, "spi": 35 },
    { "from": 7,  "to": 15, "age": 45, "lvl": 2, "intel": 30, "wmax": 35 },
    { "from": 8,  "to": 16, "age": 45, "lvl": 2, "hp": 180, "wmin": 25 },
    { "from": 9,  "to": 17, "age": 45, "lvl": 2, "intel": 28, "wmax": 25 },
    { "from": 10, "to": 18, "age": 50, "lvl": 2, "spi": 35, "wmax": 15 },
    { "from": 10, "to": 13, "age": 55, "lvl": 2, "spi": 35 },

    { "from": 11, "to": 20, "age": 100, "lvl": 4, "atk": 80, "wmin": 60 },
    { "from": 11, "to": 28, "age": 90,  "lvl": 3, "atk": 100, "wmin": 50 },
    { "from": 11, "to": 21, "age": 110, "lvl": 4, "spd": 70 },
    { "from": 12, "to": 21, "age": 100, "lvl": 4, "spd": 80, "wmin": 50 },
    { "from": 12, "to": 20, "age": 110, "lvl": 4, "atk": 80 },
    { "from": 13, "to": 22, "age": 100, "lvl": 4, "spi": 70, "wmax": 40 },
    { "from": 13, "to": 27, "age": 110, "lvl": 4, "spi": 80, "wmax": 40 },
    { "from": 14, "to": 23, "age": 100, "lvl": 4, "spi": 70, "wmax": 60 },
    { "from": 15, "to": 24, "age": 100, "lvl": 4, "intel": 70, "wmax": 60 },
    { "from": 16, "to": 25, "age": 100, "lvl": 4, "hp": 300, "wmin": 60 },
    { "from": 17, "to": 26, "age": 100, "lvl": 4, "intel": 70, "wmax": 50 },
    { "from": 18, "to": 27, "age": 100, "lvl": 4, "spi": 80, "wmax": 40 },
    { "from": 19, "to": 29, "age": 90,  "lvl": 4, "atk": 90, "wmin": 40 },
    { "from": 19, "to": 28, "age": 100, "lvl": 4, "atk": 100 },

    { "from": 20, "to": 30, "age": 200, "lvl": 7, "atk": 150, "wins": 3, "wmin": 150 },
    { "from": 20, "to": 39, "age": 260, "lvl": 10, "atk": 160, "def": 140, "wins": 8 },
    { "from": 21, "to": 31, "age": 200, "lvl": 7, "spd": 130, "wins": 3, "wmin": 120 },
    { "from": 21, "to": 39, "age": 260, "lvl": 10, "atk": 160, "def": 140, "wins": 8 },
    { "from": 22, "to": 32, "age": 200, "lvl": 7, "spi": 140, "wins": 4 },
    { "from": 23, "to": 33, "age": 200, "lvl": 7, "spi": 140, "wins": 4 },
    { "from": 24, "to": 34, "age": 200, "lvl": 7, "intel": 140, "wins": 4 },
    { "from": 25, "to": 35, "age": 200, "lvl": 7, "hp": 500, "wins": 4 },
    { "from": 26, "to": 36, "age": 200, "lvl": 7, "intel": 140, "wins": 4 },
    { "from": 27, "to": 37, "age": 200, "lvl": 7, "spi": 140, "wins": 4 },
    { "from": 28, "to": 40, "age": 180, "lvl": 7, "atk": 160, "wins": 2 },
    { "from": 29, "to": 38, "age": 200, "lvl": 7, "atk": 160, "wins": 4 }
  ]
}
)JSON";

CfgSpecies g_sp[CFG_MAX_SPECIES];  int g_spN = 0;
CfgEvo     g_evo[CFG_MAX_EVOS];    int g_evoN = 0;

static cJSON *s_root = nullptr;   // se mantiene vivo (las cadenas apuntan aqui)

static void copyStr(char *dst, size_t n, const char *src) {
  if (!src) { dst[0] = 0; return; }
  strncpy(dst, src, n - 1);
  dst[n - 1] = 0;
}

static int32_t num(cJSON *o, const char *k) {
  cJSON *v = cJSON_GetObjectItem(o, k);
  return v ? (int32_t)cJSON_GetNumberValue(v) : 0;
}

static void fallback() {
  copyStr(g_sp[0].name, sizeof(g_sp[0].name), "数码蛋");
  g_sp[0].type = DG_TYPE_UNKNOWN; g_sp[0].weight10 = 10;
  const uint16_t base0[DG_STAT_N] = { 10, 5, 1, 1, 1, 1, 1 };
  memcpy(g_sp[0].base, base0, sizeof(base0));

  copyStr(g_sp[1].name, sizeof(g_sp[1].name), "黑球兽");
  g_sp[1].type = DG_TYPE_FREE; g_sp[1].weight10 = 50;
  const uint16_t base1[DG_STAT_N] = { 45, 10, 8, 6, 4, 4, 6 };
  memcpy(g_sp[1].base, base1, sizeof(base1));

  copyStr(g_sp[2].name, sizeof(g_sp[2].name), "滚球兽");
  g_sp[2].type = DG_TYPE_FREE; g_sp[2].weight10 = 100;
  const uint16_t base2[DG_STAT_N] = { 70, 18, 16, 12, 8, 8, 14 };
  memcpy(g_sp[2].base, base2, sizeof(base2));
  g_spN = 3;
  g_evoN = 0;
}

void cfgLoad(void) {
  s_root = cJSON_Parse(CFG_JSON);
  if (!s_root) {
    ESP_LOGE(TAG, "JSON invalido -> lista de respaldo");
    fallback();
    return;
  }

  cJSON *sp = cJSON_GetObjectItem(s_root, "species");
  for (cJSON *it = sp ? sp->child : nullptr; it && g_spN < CFG_MAX_SPECIES; it = it->next) {
    CfgSpecies &s = g_sp[g_spN];
    memset(&s, 0, sizeof(s));
    copyStr(s.name, sizeof(s.name),
            cJSON_GetStringValue(cJSON_GetObjectItem(it, "name")));
    s.type = dgTypeFromName(cJSON_GetStringValue(cJSON_GetObjectItem(it, "type")));
    s.weight10 = (uint16_t)(num(it, "weight") * 10);

    cJSON *st = cJSON_GetObjectItem(it, "stats");
    for (int i = 0; i < DG_STAT_N; i++) {
      cJSON *v = st ? cJSON_GetArrayItem(st, i) : nullptr;
      s.base[i] = v ? (uint16_t)cJSON_GetNumberValue(v) : 1;
    }
    g_spN++;
  }

  cJSON *ev = cJSON_GetObjectItem(s_root, "evolve");
  for (cJSON *it = ev ? ev->child : nullptr; it && g_evoN < CFG_MAX_EVOS; it = it->next) {
    CfgEvo &e = g_evo[g_evoN];
    e.from  = (uint8_t)num(it, "from");
    e.to    = (uint8_t)num(it, "to");
    e.age   = num(it, "age");
    e.lvl   = num(it, "lvl");
    e.hp    = num(it, "hp");
    e.atk   = num(it, "atk");
    e.def   = num(it, "def");
    e.intel = num(it, "intel");
    e.spi   = num(it, "spi");
    e.spd   = num(it, "spd");
    e.wmin  = num(it, "wmin");
    e.wmax  = num(it, "wmax");
    e.wins  = num(it, "wins");
    if (e.from < g_spN && e.to < g_spN) g_evoN++;
  }

  ESP_LOGI(TAG, "config: %d especies, %d ramas", g_spN, g_evoN);
}
