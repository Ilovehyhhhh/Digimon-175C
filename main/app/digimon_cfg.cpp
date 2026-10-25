#include "digimon_cfg.h"
#include "cJSON.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "cfg";

// Formato de una especie:
//   name     nombre visible (el primer caracter hace de avatar)
//   type    疫苗 / 数据 / 病毒 / 自由 / 未知
//   weight   peso base en kg
//   stats   [HP, SP, ATK, DEF, INT, SPI, SPD]
//
// Ramas de evolucion:
//   age  minutos de vida      lvl  nivel minimo
//   hp/atk/def/intel/spi/spd  umbral de estatistica (0 = no se comprueba)
//   wmin/wmax                 rango de peso en kg (0 = sin limite)
static const char CFG_JSON[] = R"JSON(
{
  "species": [
    { "name": "黑球兽", "type": "自由", "weight": 5,
      "stats": [45, 10, 8, 6, 4, 4, 6] },
    { "name": "滚球兽", "type": "自由", "weight": 10,
      "stats": [70, 18, 16, 12, 8, 8, 14] },
    { "name": "亚古兽", "type": "病毒", "weight": 35,
      "stats": [120, 38, 34, 22, 14, 16, 26] },
    { "name": "加布兽", "type": "数据", "weight": 40,
      "stats": [112, 36, 30, 26, 16, 18, 24] },
    { "name": "巴达兽", "type": "疫苗", "weight": 14,
      "stats": [95, 42, 20, 16, 22, 28, 28] },
    { "name": "暴龙兽", "type": "病毒", "weight": 120,
      "stats": [230, 62, 60, 44, 24, 28, 40] },
    { "name": "加鲁鲁兽", "type": "数据", "weight": 95,
      "stats": [210, 60, 52, 42, 30, 34, 46] },
    { "name": "天使兽", "type": "疫苗", "weight": 32,
      "stats": [175, 82, 34, 32, 46, 56, 48] },
    { "name": "机械暴龙兽", "type": "病毒", "weight": 260,
      "stats": [390, 92, 94, 78, 42, 54, 62] },
    { "name": "兽人加鲁鲁", "type": "数据", "weight": 185,
      "stats": [350, 90, 82, 66, 48, 58, 78] },
    { "name": "神圣天使兽", "type": "疫苗", "weight": 75,
      "stats": [330, 142, 60, 58, 88, 100, 74] },
    { "name": "战斗暴龙兽", "type": "病毒", "weight": 400,
      "stats": [570, 132, 142, 112, 66, 82, 94] },
    { "name": "钢铁加鲁鲁", "type": "数据", "weight": 330,
      "stats": [530, 130, 126, 120, 80, 94, 106] },
    { "name": "究极天使兽", "type": "疫苗", "weight": 155,
      "stats": [490, 205, 94, 90, 134, 152, 102] },
    { "name": "丧尸暴龙兽", "type": "未知", "weight": 310,
      "stats": [430, 72, 152, 62, 20, 32, 52] }
  ],
  "evolve": [
    { "from": 0, "to": 1, "age": 10 },
    { "from": 1, "to": 2, "age": 25, "lvl": 2, "wmin": 12 },
    { "from": 1, "to": 3, "age": 25, "wmax": 11 },
    { "from": 1, "to": 4, "age": 30, "lvl": 2 },
    { "from": 2, "to": 5, "age": 60, "lvl": 3 },
    { "from": 3, "to": 6, "age": 60, "lvl": 3 },
    { "from": 4, "to": 7, "age": 60, "lvl": 3 },
    { "from": 5, "to": 8, "age": 120, "lvl": 5 },
    { "from": 6, "to": 9, "age": 120, "lvl": 5 },
    { "from": 7, "to": 10, "age": 120, "lvl": 5 },
    { "from": 8, "to": 11, "age": 200, "lvl": 7 },
    { "from": 9, "to": 12, "age": 200, "lvl": 7 },
    { "from": 10, "to": 13, "age": 200, "lvl": 7 },
    { "from": 5, "to": 14, "age": 90, "lvl": 4, "atk": 90 }
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
  copyStr(g_sp[0].name, sizeof(g_sp[0].name), "黑球兽");
  g_sp[0].type = DG_TYPE_FREE; g_sp[0].weight10 = 50;
  const uint16_t base0[DG_STAT_N] = { 45, 10, 8, 6, 4, 4, 6 };
  memcpy(g_sp[0].base, base0, sizeof(base0));

  copyStr(g_sp[1].name, sizeof(g_sp[1].name), "滚球兽");
  g_sp[1].type = DG_TYPE_FREE; g_sp[1].weight10 = 100;
  const uint16_t base1[DG_STAT_N] = { 70, 18, 16, 12, 8, 8, 14 };
  memcpy(g_sp[1].base, base1, sizeof(base1));

  copyStr(g_sp[2].name, sizeof(g_sp[2].name), "亚古兽");
  g_sp[2].type = DG_TYPE_VIRUS; g_sp[2].weight10 = 350;
  const uint16_t base2[DG_STAT_N] = { 120, 38, 34, 22, 14, 16, 26 };
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
    if (e.from < g_spN && e.to < g_spN) g_evoN++;
  }

  ESP_LOGI(TAG, "config: %d especies, %d ramas", g_spN, g_evoN);
}
