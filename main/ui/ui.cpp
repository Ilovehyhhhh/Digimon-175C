#include "ui.h"

#include "app/battle.h"
#include "app/digimon_cfg.h"
#include "board.h"
#include "clock.h"
#include "imu.h"
#include "pet_avatar.h"
#include "power.h"

#include "esp_err.h"
#include "esp_pm.h"

#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include <Arduino.h>
#include <Preferences.h>

#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(digifont);
LV_FONT_DECLARE(digifont_small);
LV_FONT_DECLARE(digifont_big);

namespace {

constexpr uint32_t C_BG     = 0x080C12;
constexpr uint32_t C_PANEL  = 0x1E2A3A;
constexpr uint32_t C_TEXT   = 0xE8F1FA;
constexpr uint32_t C_DIM    = 0x7E90A6;
constexpr uint32_t C_ACCENT = 0x4FC3F7;
constexpr uint32_t C_GOOD   = 0x66BB6A;
constexpr uint32_t C_WARN   = 0xFFA726;
constexpr uint32_t C_BAD    = 0xEF5350;
constexpr uint32_t TRAIN_MS = 10000;
constexpr uint32_t UI_REFR_PERIOD_MS = 15;
constexpr uint32_t LOCK_REFR_PERIOD_MS = 500;

enum UiAct : uintptr_t {
  ACT_NONE = 0,
  ACT_EVOLVE, ACT_TRAIN, ACT_BATTLE, ACT_STATUS,
  ACT_TYPES, ACT_SYSTEM, ACT_HOME, ACT_RESET,
  ACT_TIMESET, ACT_TIME_HM, ACT_TIME_HP, ACT_TIME_MM, ACT_TIME_MP,
};

Digimon *g_pet = nullptr;
Preferences s_ui;

bool g_locked = false;
bool g_training = false;
bool g_resetConfirm = false;
uint8_t g_bright = 100;

uint32_t g_toastUntil = 0;
uint32_t g_trainEnd = 0;
uint32_t g_lastSave = 0;
uint32_t g_lastLockBat = 0;
uint32_t g_lastClockSave = 0;
uint32_t g_animCount = 0;
uint16_t g_trainHits = 0;
char g_toast[64] = "";

Battle g_battle;

lv_obj_t *scr_home = nullptr;
lv_obj_t *scr_menu = nullptr;
lv_obj_t *scr_evolve = nullptr;
lv_obj_t *scr_status = nullptr;
lv_obj_t *scr_types = nullptr;
lv_obj_t *scr_system = nullptr;
lv_obj_t *scr_time = nullptr;
lv_obj_t *scr_train = nullptr;
lv_obj_t *scr_battle = nullptr;
lv_obj_t *scr_lock = nullptr;

PetAvatar g_avatar;

lv_obj_t *lblName = nullptr;
lv_obj_t *lblSub = nullptr;
lv_obj_t *lblType = nullptr;
lv_obj_t *lblToast = nullptr;
lv_obj_t *lblHp = nullptr;
lv_obj_t *lblSp = nullptr;
lv_obj_t *lblHpVal = nullptr;
lv_obj_t *lblSpVal = nullptr;
lv_obj_t *hpBar = nullptr;
lv_obj_t *spBar = nullptr;
lv_obj_t *statLbl[6] = {};

lv_obj_t *evolveList = nullptr;
lv_obj_t *lblStatus = nullptr;
lv_obj_t *lblBrightVal = nullptr;
lv_obj_t *sliderBright = nullptr;
lv_obj_t *lblSysInfo = nullptr;
lv_obj_t *lblKeys = nullptr;
lv_obj_t *lblSysMsg = nullptr;

lv_obj_t *lblTrainCount = nullptr;
lv_obj_t *lblTrainTime = nullptr;
lv_obj_t *lblTrainResult = nullptr;

lv_obj_t *lblFoeName = nullptr;
lv_obj_t *lblFoeInfo = nullptr;
lv_obj_t *lblYourMove = nullptr;
lv_obj_t *lblFoeMove = nullptr;
lv_obj_t *lblMeName = nullptr;
lv_obj_t *lblMeInfo = nullptr;
lv_obj_t *meHpBar = nullptr;
lv_obj_t *meSpBar = nullptr;
lv_obj_t *lblBattleResult = nullptr;
lv_obj_t *battleBtn[3] = {};
lv_obj_t *btnBattleBack = nullptr;

lv_obj_t *lblLockName = nullptr;
lv_obj_t *lblLockInfo = nullptr;
lv_obj_t *lblLockTime = nullptr;
lv_obj_t *lblLockPct = nullptr;
lv_obj_t *lblLockChg = nullptr;
lv_obj_t *lockBatFill = nullptr;

lv_obj_t *lblTimeSet = nullptr;

lv_display_t *g_disp = nullptr;
lv_timer_t *g_animTimer = nullptr;

void onAction(lv_event_t *e);
void onBattleAction(lv_event_t *e);
void onEvolveTarget(lv_event_t *e);
void onDevolveTarget(lv_event_t *e);
void onBright(lv_event_t *e);
void refreshHome();
void goHome();
void openTimeSet();
void refreshTimeSet();
void updateLockTime();
void updateLockBattery();
void setPm(bool locked);

// ---------------------------------------------------------------- helpers

uint32_t typeColor(uint8_t t) {
  switch (t) {
    case DG_TYPE_VACCINE: return 0x4DD0E1;
    case DG_TYPE_DATA:    return 0x64B5F6;
    case DG_TYPE_VIRUS:   return 0xBA68C8;
    case DG_TYPE_FREE:    return 0x81C784;
    default:              return 0x9E9E9E;
  }
}

uint32_t hpColor(uint16_t hp, uint16_t max) {
  if (max == 0) return C_BAD;
  uint32_t pct = (uint32_t)hp * 100 / max;
  if (pct > 50) return C_GOOD;
  if (pct > 25) return C_WARN;
  return C_BAD;
}

int moodFromHp() {
  if (!g_pet || g_pet->maxHp() == 0) return 2;
  uint32_t pct = (uint32_t)g_pet->hp * 100 / g_pet->maxHp();
  if (pct >= 70) return 4;
  if (pct >= 40) return 3;
  if (pct >= 15) return 1;
  return 0;
}

void ageText(char *out, size_t n, uint32_t minutes) {
  if (minutes < 60) snprintf(out, n, "%u分钟", (unsigned)minutes);
  else if (minutes < 1440) snprintf(out, n, "%u小时%u分", (unsigned)(minutes / 60), (unsigned)(minutes % 60));
  else snprintf(out, n, "%u天%u小时", (unsigned)(minutes / 1440), (unsigned)((minutes % 1440) / 60));
}

void toast(const char *msg) {
  strncpy(g_toast, msg, sizeof(g_toast) - 1);
  g_toast[sizeof(g_toast) - 1] = 0;
  g_toastUntil = millis() + 2200;
}

lv_obj_t *mkScreen() {
  lv_obj_t *s = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s, lv_color_hex(C_BG), 0);
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(s, 0, 0);
  lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
  return s;
}

lv_obj_t *mkLabel(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, text ? text : "");
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  return l;
}

lv_obj_t *mkBtnBase(lv_obj_t *parent, const char *text, int w, int h,
                    const lv_font_t *font, uint32_t bg = C_PANEL) {
  lv_obj_t *b = lv_button_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_radius(b, h >= 40 ? h / 2 : 10, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_t *l = mkLabel(b, text, font, C_TEXT);
  lv_obj_center(l);
  return b;
}

lv_obj_t *mkButton(lv_obj_t *parent, const char *text, int w, int h,
                   const lv_font_t *font, UiAct act, uint32_t bg = C_PANEL) {
  lv_obj_t *b = mkBtnBase(parent, text, w, h, font, bg);
  lv_obj_add_event_cb(b, onAction, LV_EVENT_CLICKED, (void *)(uintptr_t)act);
  return b;
}

void applyBrightness() {
  bsp_display_brightness_set(g_bright);
}

void goHome() {
  g_training = false;
  g_resetConfirm = false;
  lv_screen_load(scr_home);
  refreshHome();
}

// ---------------------------------------------------------------- home

void refreshHome() {
  if (!g_pet) return;

  lv_label_set_text(lblName, g_pet->name());

  char age[32];
  ageText(age, sizeof(age), g_pet->ageMin);
  PowerBatt b = powerBattery();
  if (b.pct >= 0) lv_label_set_text_fmt(lblSub, "Lv.%u  %s  %d%%", (unsigned)g_pet->level(), age, b.pct);
  else lv_label_set_text_fmt(lblSub, "Lv.%u  %s", (unsigned)g_pet->level(), age);

  lv_label_set_text_fmt(lblType, "属性 %s", dgTypeName(g_pet->type()));
  lv_obj_set_style_text_color(lblType, lv_color_hex(typeColor(g_pet->type())), 0);

  uint16_t hpmax = g_pet->maxHp(), spmax = g_pet->maxSp();
  lv_bar_set_range(hpBar, 0, hpmax ? hpmax : 1);
  lv_bar_set_value(hpBar, g_pet->hp, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(hpBar, lv_color_hex(hpColor(g_pet->hp, hpmax)), LV_PART_INDICATOR);
  lv_label_set_text_fmt(lblHpVal, "%u/%u", (unsigned)g_pet->hp, (unsigned)hpmax);

  lv_bar_set_range(spBar, 0, spmax ? spmax : 1);
  lv_bar_set_value(spBar, g_pet->sp, LV_ANIM_OFF);
  lv_label_set_text_fmt(lblSpVal, "%u/%u", (unsigned)g_pet->sp, (unsigned)spmax);

  lv_label_set_text_fmt(statLbl[0], "攻击 %u", (unsigned)g_pet->atk());
  lv_label_set_text_fmt(statLbl[1], "防御 %u", (unsigned)g_pet->def());
  lv_label_set_text_fmt(statLbl[2], "智力 %u", (unsigned)g_pet->intel());
  lv_label_set_text_fmt(statLbl[3], "精神 %u", (unsigned)g_pet->spi());
  lv_label_set_text_fmt(statLbl[4], "速度 %u", (unsigned)g_pet->spd());
  uint16_t w = g_pet->weight10();
  lv_label_set_text_fmt(statLbl[5], "体重 %u.%ukg", (unsigned)(w / 10), (unsigned)(w % 10));

  petAvatarSetSpecies(g_avatar, g_pet->name(), typeColor(g_pet->type()));
  uint8_t act = g_pet->act(millis());
  bool excited = (act == DG_ACT_HAPPY || act == DG_ACT_TRAIN || act == DG_ACT_ATTACK);
  petAvatarSetMood(g_avatar, moodFromHp(), excited);
  petAvatarSetImage(g_avatar, g_petImage);

  if ((int32_t)(millis() - g_toastUntil) < 0 && g_toast[0]) {
    lv_label_set_text(lblToast, g_toast);
    lv_obj_set_style_text_color(lblToast, lv_color_hex(C_WARN), 0);
  } else {
    g_toast[0] = 0;
    lv_label_set_text(lblToast, "");
  }
}

void buildHome() {
  scr_home = mkScreen();

  lblName = mkLabel(scr_home, "", &digifont, C_TEXT);
  lv_obj_align(lblName, LV_ALIGN_TOP_MID, 0, 12);

  lblSub = mkLabel(scr_home, "", &digifont_small, C_DIM);
  lv_obj_align(lblSub, LV_ALIGN_TOP_MID, 0, 46);

  lblType = mkLabel(scr_home, "", &digifont_small, C_DIM);
  lv_obj_align(lblType, LV_ALIGN_TOP_MID, 0, 70);

  petAvatarCreate(g_avatar, scr_home, 156, -56);

  lblHp = mkLabel(scr_home, "HP", &digifont_small, C_TEXT);
  lv_obj_set_pos(lblHp, 56, 254);
  lv_obj_set_width(lblHp, 44);
  lv_obj_set_style_text_align(lblHp, LV_TEXT_ALIGN_LEFT, 0);

  hpBar = lv_bar_create(scr_home);
  lv_obj_set_size(hpBar, 196, 10);
  lv_obj_set_pos(hpBar, 104, 260);
  lv_obj_set_style_bg_color(hpBar, lv_color_hex(C_PANEL), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(hpBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(hpBar, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(hpBar, lv_color_hex(C_GOOD), LV_PART_INDICATOR);
  lv_obj_set_style_radius(hpBar, 5, LV_PART_INDICATOR);

  lblHpVal = mkLabel(scr_home, "", &digifont_small, C_DIM);
  lv_obj_set_pos(lblHpVal, 308, 254);
  lv_obj_set_width(lblHpVal, 100);
  lv_obj_set_style_text_align(lblHpVal, LV_TEXT_ALIGN_LEFT, 0);

  lblSp = mkLabel(scr_home, "SP", &digifont_small, C_TEXT);
  lv_obj_set_pos(lblSp, 56, 278);
  lv_obj_set_width(lblSp, 44);
  lv_obj_set_style_text_align(lblSp, LV_TEXT_ALIGN_LEFT, 0);

  spBar = lv_bar_create(scr_home);
  lv_obj_set_size(spBar, 196, 10);
  lv_obj_set_pos(spBar, 104, 284);
  lv_obj_set_style_bg_color(spBar, lv_color_hex(C_PANEL), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(spBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(spBar, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(spBar, lv_color_hex(C_ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_radius(spBar, 5, LV_PART_INDICATOR);

  lblSpVal = mkLabel(scr_home, "", &digifont_small, C_DIM);
  lv_obj_set_pos(lblSpVal, 308, 278);
  lv_obj_set_width(lblSpVal, 100);
  lv_obj_set_style_text_align(lblSpVal, LV_TEXT_ALIGN_LEFT, 0);

  lblToast = mkLabel(scr_home, "", &digifont_small, C_WARN);
  lv_obj_set_width(lblToast, 400);
  lv_obj_align(lblToast, LV_ALIGN_TOP_MID, 0, 298);

  const char *stats[6] = { "攻击", "防御", "智力", "精神", "速度", "体重" };
  for (int i = 0; i < 6; i++) {
    int col = i % 3, row = i / 3;
    statLbl[i] = mkLabel(scr_home, stats[i], &digifont_small, C_TEXT);
    lv_obj_set_pos(statLbl[i], 52 + col * 120, 318 + row * 26);
    lv_obj_set_width(statLbl[i], 112);
    lv_obj_set_style_text_align(statLbl[i], LV_TEXT_ALIGN_LEFT, 0);
  }

  lv_obj_t *train = mkButton(scr_home, "训练", 110, 40, &digifont, ACT_TRAIN);
  lv_obj_set_pos(train, 113, 368);
  lv_obj_t *battle = mkButton(scr_home, "对战", 110, 40, &digifont, ACT_BATTLE);
  lv_obj_set_pos(battle, 243, 368);
}

// ---------------------------------------------------------------- menu

void openMenu() {
  lv_screen_load(scr_menu);
}

void buildMenu() {
  scr_menu = mkScreen();
  lv_obj_t *t = mkLabel(scr_menu, "菜单", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 24);

  const char *names[7] = { "进化", "训练", "对战", "状态", "属性", "系统", "返回" };
  const UiAct acts[7] = { ACT_EVOLVE, ACT_TRAIN, ACT_BATTLE, ACT_STATUS,
                          ACT_TYPES, ACT_SYSTEM, ACT_HOME };
  for (int i = 0; i < 7; i++) {
    lv_obj_t *b = mkButton(scr_menu, names[i], 220, 42, &digifont, acts[i]);
    lv_obj_set_pos(b, 123, 78 + i * 46);
  }
}

// ---------------------------------------------------------------- status

void refreshStatus() {
  if (!g_pet) return;
  PowerBatt b = powerBattery();
  char age[32];
  ageText(age, sizeof(age), g_pet->ageMin);
  uint16_t w = g_pet->weight10();

  char txt[640];
  snprintf(txt, sizeof(txt),
           "名字  %s  属性 %s\n"
           "等级  Lv.%u  年龄 %s\n"
           "体重  %u.%ukg  经验 %u\n"
           "战力  %u\n"
           "HP    %u/%u\n"
           "SP    %u/%u\n"
           "攻击  %u  +%u\n"
           "防御  %u  +%u\n"
           "智力  %u  +%u\n"
           "精神  %u  +%u\n"
           "速度  %u  +%u\n"
           "电量  %d%%%s",
           g_pet->name(), dgTypeName(g_pet->type()),
           (unsigned)g_pet->level(), age,
           (unsigned)(w / 10), (unsigned)(w % 10), (unsigned)g_pet->exp,
           (unsigned)g_pet->power(),
           (unsigned)g_pet->hp, (unsigned)g_pet->maxHp(),
           (unsigned)g_pet->sp, (unsigned)g_pet->maxSp(),
           (unsigned)g_pet->atk(), (unsigned)g_pet->train[DG_STAT_ATK],
           (unsigned)g_pet->def(), (unsigned)g_pet->train[DG_STAT_DEF],
           (unsigned)g_pet->intel(), (unsigned)g_pet->train[DG_STAT_INT],
           (unsigned)g_pet->spi(), (unsigned)g_pet->train[DG_STAT_SPI],
           (unsigned)g_pet->spd(), (unsigned)g_pet->train[DG_STAT_SPD],
           b.pct < 0 ? 0 : b.pct, b.usb ? " 充电中" : "");
  lv_label_set_text(lblStatus, txt);
}

void openStatus() {
  refreshStatus();
  lv_screen_load(scr_status);
}

void buildStatus() {
  scr_status = mkScreen();
  lv_obj_t *t = mkLabel(scr_status, "状态", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 20);

  lblStatus = mkLabel(scr_status, "", &digifont_small, C_TEXT);
  lv_obj_set_pos(lblStatus, 62, 60);
  lv_obj_set_width(lblStatus, 342);
  lv_obj_set_style_text_align(lblStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_line_space(lblStatus, 4, 0);

  lv_obj_t *back = mkButton(scr_status, "返回", 120, 40, &digifont_small, ACT_HOME);
  lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -22);
}

// ---------------------------------------------------------------- types

void buildTypes() {
  scr_types = mkScreen();
  lv_obj_t *t = mkLabel(scr_types, "属性克制", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 24);

  struct Row { const char *a; const char *b; };
  const Row rows[3] = {
    { "疫苗", "病毒" }, { "病毒", "数据" }, { "数据", "疫苗" },
  };
  for (int i = 0; i < 3; i++) {
    int y = 96 + i * 52;
    lv_obj_t *la = mkLabel(scr_types, rows[i].a, &digifont, C_TEXT);
    lv_obj_set_pos(la, 86, y);
    lv_obj_set_width(la, 90);
    lv_obj_set_style_text_align(la, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(la, lv_color_hex(typeColor(
        dgTypeFromName(rows[i].a))), 0);

    lv_obj_t *mid = mkLabel(scr_types, "克", &digifont, C_DIM);
    lv_obj_set_pos(mid, 196, y);
    lv_obj_set_width(mid, 60);

    lv_obj_t *lb = mkLabel(scr_types, rows[i].b, &digifont, C_TEXT);
    lv_obj_set_pos(lb, 256, y);
    lv_obj_set_width(lb, 90);
    lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_color(lb, lv_color_hex(typeColor(
        dgTypeFromName(rows[i].b))), 0);
  }

  lv_obj_t *free = mkLabel(scr_types, "自由  不参与三角克制", &digifont_small, typeColor(DG_TYPE_FREE));
  lv_obj_set_width(free, 360);
  lv_obj_align(free, LV_ALIGN_TOP_MID, 0, 268);

  lv_obj_t *unk = mkLabel(scr_types, "未知  不参与三角克制", &digifont_small, typeColor(DG_TYPE_UNKNOWN));
  lv_obj_set_width(unk, 360);
  lv_obj_align(unk, LV_ALIGN_TOP_MID, 0, 296);

  lv_obj_t *note = mkLabel(scr_types, "效果绝佳 150%  效果不佳 50%", &digifont_small, C_DIM);
  lv_obj_set_width(note, 400);
  lv_obj_align(note, LV_ALIGN_TOP_MID, 0, 334);

  lv_obj_t *back = mkButton(scr_types, "返回", 120, 40, &digifont_small, ACT_HOME);
  lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -22);
}

// ---------------------------------------------------------------- system

void refreshSystemInfo() {
  if (!g_pet) return;
  PowerBatt b = powerBattery();
  uint32_t sec = millis() / 1000;
  char uptime[32];
  if (sec < 3600) snprintf(uptime, sizeof(uptime), "%u分%u秒", (unsigned)(sec / 60), (unsigned)(sec % 60));
  else snprintf(uptime, sizeof(uptime), "%u小时%u分", (unsigned)(sec / 3600), (unsigned)((sec / 60) % 60));

  char info[220];
  snprintf(info, sizeof(info),
           "电量  %d%%%s\n运行  %s\n设备  ESP32-S3 AMOLED 1.75C",
           b.pct < 0 ? 0 : b.pct, b.usb ? "  充电中" : "", uptime);
  lv_label_set_text(lblSysInfo, info);
}

void openSystem() {
  g_resetConfirm = false;
  lv_slider_set_value(sliderBright, g_bright, LV_ANIM_OFF);
  lv_label_set_text_fmt(lblBrightVal, "%u%%", (unsigned)g_bright);
  lv_label_set_text(lblSysMsg, "");
  refreshSystemInfo();
  lv_screen_load(scr_system);
}

void onBright(lv_event_t *e) {
  lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
  int v = (int)lv_slider_get_value(s);
  if (v < 10) v = 10;
  g_bright = (uint8_t)v;
  lv_label_set_text_fmt(lblBrightVal, "%d%%", v);
  if (!g_locked) bsp_display_brightness_set(g_bright);
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    s_ui.putUChar("bright", g_bright);
    s_ui.commit();
  }
}

void buildSystem() {
  scr_system = mkScreen();
  lv_obj_t *t = mkLabel(scr_system, "系统", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 18);

  lv_obj_t *bl = mkLabel(scr_system, "亮度", &digifont, C_TEXT);
  lv_obj_set_pos(bl, 70, 68);
  lv_obj_set_width(bl, 90);
  lv_obj_set_style_text_align(bl, LV_TEXT_ALIGN_LEFT, 0);

  lblBrightVal = mkLabel(scr_system, "100%", &digifont, C_ACCENT);
  lv_obj_set_pos(lblBrightVal, 320, 68);
  lv_obj_set_width(lblBrightVal, 80);
  lv_obj_set_style_text_align(lblBrightVal, LV_TEXT_ALIGN_RIGHT, 0);

  sliderBright = lv_slider_create(scr_system);
  lv_obj_set_size(sliderBright, 326, 18);
  lv_obj_set_pos(sliderBright, 70, 108);
  lv_slider_set_range(sliderBright, 10, 100);
  lv_slider_set_value(sliderBright, g_bright, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(sliderBright, lv_color_hex(C_PANEL), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sliderBright, lv_color_hex(C_ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sliderBright, lv_color_hex(C_TEXT), LV_PART_KNOB);
  lv_obj_add_event_cb(sliderBright, onBright, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(sliderBright, onBright, LV_EVENT_RELEASED, nullptr);

  lblSysInfo = mkLabel(scr_system, "", &digifont_small, C_DIM);
  lv_obj_set_pos(lblSysInfo, 70, 144);
  lv_obj_set_width(lblSysInfo, 326);
  lv_obj_set_style_text_align(lblSysInfo, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_line_space(lblSysInfo, 6, 0);

  lblKeys = mkLabel(scr_system, "BOOT 短按  菜单\nPWR 短按   锁屏\nPWR 长按   关机",
                    &digifont_small, C_DIM);
  lv_obj_set_pos(lblKeys, 70, 230);
  lv_obj_set_width(lblKeys, 326);
  lv_obj_set_style_text_align(lblKeys, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_line_space(lblKeys, 4, 0);

  lv_obj_t *ts = mkButton(scr_system, "校时", 110, 40, &digifont_small, ACT_TIMESET);
  lv_obj_set_pos(ts, 44, 352);
  lv_obj_t *rst = mkButton(scr_system, "重置存档", 110, 40, &digifont_small, ACT_RESET, 0x5A1F1F);
  lv_obj_set_pos(rst, 178, 352);
  lv_obj_t *back = mkButton(scr_system, "返回", 110, 40, &digifont_small, ACT_HOME);
  lv_obj_set_pos(back, 312, 352);

  lblSysMsg = mkLabel(scr_system, "", &digifont_small, C_WARN);
  lv_obj_set_width(lblSysMsg, 380);
  lv_obj_align(lblSysMsg, LV_ALIGN_TOP_MID, 0, 398);
}

// ---------------------------------------------------------------- evolve

void openEvolve() {
  lv_obj_clean(evolveList);

  lv_obj_t *cur = mkLabel(evolveList, "", &digifont_small, C_DIM);
  lv_obj_set_width(cur, 340);
  lv_obj_set_style_text_align(cur, LV_TEXT_ALIGN_CENTER, 0);
  char line[64];
  snprintf(line, sizeof(line), "当前  %s", g_pet->name());
  lv_label_set_text(cur, line);

  lv_obj_t *eh = mkLabel(evolveList, "可进化", &digifont, C_TEXT);
  lv_obj_set_width(eh, 340);

  uint8_t opts[CFG_MAX_SPECIES];
  int n = g_pet->evoList(opts, CFG_MAX_SPECIES);
  if (n == 0) {
    lv_obj_t *l = mkLabel(evolveList, "条件不足", &digifont_small, C_DIM);
    lv_obj_set_width(l, 340);
  } else {
    for (int i = 0; i < n; i++) {
      char label[64];
      snprintf(label, sizeof(label), "%s  %s", g_sp[opts[i]].name,
               dgTypeName(g_sp[opts[i]].type));
      lv_obj_t *b = mkBtnBase(evolveList, label, 340, 48, &digifont);
      lv_obj_add_event_cb(b, onEvolveTarget, LV_EVENT_CLICKED, (void *)(uintptr_t)opts[i]);
    }
  }

  lv_obj_t *dh = mkLabel(evolveList, "可退化", &digifont, C_TEXT);
  lv_obj_set_width(dh, 340);

  uint8_t dopt[CFG_MAX_SPECIES];
  int dn = g_pet->devoList(dopt, CFG_MAX_SPECIES);
  if (dn == 0) {
    lv_obj_t *l = mkLabel(evolveList, "已是最初形态", &digifont_small, C_DIM);
    lv_obj_set_width(l, 340);
  } else {
    for (int i = 0; i < dn; i++) {
      char label[64];
      snprintf(label, sizeof(label), "退化  %s", g_sp[dopt[i]].name);
      lv_obj_t *b = mkBtnBase(evolveList, label, 340, 44, &digifont);
      lv_obj_add_event_cb(b, onDevolveTarget, LV_EVENT_CLICKED, (void *)(uintptr_t)dopt[i]);
    }
  }

  lv_screen_load(scr_evolve);
}

void buildEvolve() {
  scr_evolve = mkScreen();
  lv_obj_t *t = mkLabel(scr_evolve, "进化", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 18);

  evolveList = lv_obj_create(scr_evolve);
  lv_obj_set_size(evolveList, 360, 306);
  lv_obj_set_pos(evolveList, 53, 62);
  lv_obj_set_style_bg_opa(evolveList, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(evolveList, 0, 0);
  lv_obj_set_style_pad_all(evolveList, 0, 0);
  lv_obj_set_style_pad_row(evolveList, 6, 0);
  lv_obj_set_flex_flow(evolveList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(evolveList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *back = mkButton(scr_evolve, "返回", 120, 40, &digifont_small, ACT_HOME);
  lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -18);
}

// ---------------------------------------------------------------- training

void addTrainHit() {
  if (!g_training) return;
  g_trainHits++;
  lv_label_set_text_fmt(lblTrainCount, "%u 次", (unsigned)g_trainHits);
}

void finishTraining() {
  g_training = false;
  char summary[64];
  g_pet->trainGame(g_trainHits, summary, sizeof(summary));
  g_pet->save();
  lv_label_set_text(lblTrainTime, "0");
  lv_label_set_text(lblTrainResult, summary);
  refreshHome();
}

void openTraining() {
  g_training = true;
  g_trainHits = 0;
  g_trainEnd = millis() + TRAIN_MS;
  lv_label_set_text(lblTrainCount, "0 次");
  lv_label_set_text(lblTrainTime, "10");
  lv_label_set_text(lblTrainResult, "摇一摇或连续点击");
  lv_screen_load(scr_train);
}

void buildTraining() {
  scr_train = mkScreen();
  lv_obj_t *t = mkLabel(scr_train, "训练", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 26);

  lblTrainTime = mkLabel(scr_train, "10", &digifont_big, C_ACCENT);
  lv_obj_align(lblTrainTime, LV_ALIGN_TOP_MID, 0, 70);

  lv_obj_t *unit = mkLabel(scr_train, "秒", &digifont_small, C_DIM);
  lv_obj_align(unit, LV_ALIGN_TOP_MID, 0, 140);

  lblTrainCount = mkLabel(scr_train, "0 次", &digifont, C_TEXT);
  lv_obj_align(lblTrainCount, LV_ALIGN_TOP_MID, 0, 166);

  lv_obj_t *tap = mkBtnBase(scr_train, "摇一摇 / 点我", 280, 104, &digifont);
  lv_obj_align(tap, LV_ALIGN_CENTER, 0, 36);
  lv_obj_add_event_cb(tap, [](lv_event_t *e) {
    lv_event_stop_bubbling(e);
    addTrainHit();
  }, LV_EVENT_CLICKED, nullptr);

  lblTrainResult = mkLabel(scr_train, "摇一摇或连续点击", &digifont_small, C_DIM);
  lv_obj_set_width(lblTrainResult, 380);
  lv_obj_align(lblTrainResult, LV_ALIGN_TOP_MID, 0, 332);

  lv_obj_t *back = mkButton(scr_train, "返回", 120, 40, &digifont_small, ACT_HOME);
  lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -18);
}

// ---------------------------------------------------------------- battle

void refreshBattle() {
  Battle &b = g_battle;

  lv_label_set_text(lblFoeName, b.foe.name);
  lv_label_set_text_fmt(lblFoeInfo, "Lv.%u  %s",
                        (unsigned)b.foe.level, dgTypeName(b.foe.type));
  lv_obj_set_style_text_color(lblFoeInfo, lv_color_hex(typeColor(b.foe.type)), 0);

  lv_label_set_text(lblMeName, b.me.name);
  lv_label_set_text_fmt(lblMeInfo, "Lv.%u  %s  HP %u/%u  SP %u/%u",
                        (unsigned)b.me.level, dgTypeName(b.me.type),
                        (unsigned)b.me.hp, (unsigned)b.me.hpMax,
                        (unsigned)b.me.sp, (unsigned)b.me.spMax);
  lv_obj_set_style_text_color(lblMeInfo, lv_color_hex(typeColor(b.me.type)), 0);

  lv_bar_set_range(meHpBar, 0, b.me.hpMax ? b.me.hpMax : 1);
  lv_bar_set_value(meHpBar, b.me.hp, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(meHpBar, lv_color_hex(hpColor(b.me.hp, b.me.hpMax)),
                            LV_PART_INDICATOR);
  lv_bar_set_range(meSpBar, 0, b.me.spMax ? b.me.spMax : 1);
  lv_bar_set_value(meSpBar, b.me.sp, LV_ANIM_OFF);

  bool over = (b.state != BS_ACTIVE);
  if (!over) {
    lv_label_set_text(lblYourMove, "你出: ?");
    lv_label_set_text(lblFoeMove, "对手出: ?");
    lv_label_set_text(lblBattleResult, "选择招式");
    lv_obj_set_style_text_color(lblBattleResult, lv_color_hex(C_DIM), 0);
  } else {
    lv_label_set_text_fmt(lblYourMove, "你出: %s", rpsName(b.playerMove));
    lv_label_set_text_fmt(lblFoeMove, "对手出: %s", rpsName(b.foeMove));
    const char *res = "平局";
    uint32_t col = C_WARN;
    if (b.state == BS_WIN) { res = "你赢了"; col = C_GOOD; }
    else if (b.state == BS_LOSE) { res = "你输了"; col = C_BAD; }
    lv_label_set_text(lblBattleResult, res);
    lv_obj_set_style_text_color(lblBattleResult, lv_color_hex(col), 0);
  }

  for (int i = 0; i < 3; i++) {
    if (over) lv_obj_add_flag(battleBtn[i], LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(battleBtn[i], LV_OBJ_FLAG_HIDDEN);
  }
  if (over) lv_obj_remove_flag(btnBattleBack, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(btnBattleBack, LV_OBJ_FLAG_HIDDEN);
}

void openBattle() {
  if (!g_pet) return;
  battleBegin(g_battle, *g_pet);
  refreshBattle();
  lv_screen_load(scr_battle);
}

void buildBattle() {
  scr_battle = mkScreen();

  lblFoeName = mkLabel(scr_battle, "", &digifont, C_TEXT);
  lv_obj_align(lblFoeName, LV_ALIGN_TOP_MID, 0, 12);

  lblFoeInfo = mkLabel(scr_battle, "", &digifont_small, C_DIM);
  lv_obj_align(lblFoeInfo, LV_ALIGN_TOP_MID, 0, 46);

  lblYourMove = mkLabel(scr_battle, "你出: ?", &digifont, C_TEXT);
  lv_obj_set_width(lblYourMove, 400);
  lv_obj_align(lblYourMove, LV_ALIGN_TOP_MID, 0, 92);

  lblFoeMove = mkLabel(scr_battle, "对手出: ?", &digifont, C_TEXT);
  lv_obj_set_width(lblFoeMove, 400);
  lv_obj_align(lblFoeMove, LV_ALIGN_TOP_MID, 0, 128);

  lblBattleResult = mkLabel(scr_battle, "选择招式", &digifont, C_DIM);
  lv_obj_set_width(lblBattleResult, 400);
  lv_obj_align(lblBattleResult, LV_ALIGN_TOP_MID, 0, 176);

  lblMeName = mkLabel(scr_battle, "", &digifont, C_TEXT);
  lv_obj_align(lblMeName, LV_ALIGN_TOP_MID, 0, 224);

  lblMeInfo = mkLabel(scr_battle, "", &digifont_small, C_DIM);
  lv_obj_align(lblMeInfo, LV_ALIGN_TOP_MID, 0, 258);

  meHpBar = lv_bar_create(scr_battle);
  lv_obj_set_size(meHpBar, 280, 10);
  lv_obj_set_pos(meHpBar, 93, 286);
  lv_obj_set_style_bg_color(meHpBar, lv_color_hex(C_PANEL), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(meHpBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(meHpBar, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(meHpBar, lv_color_hex(C_GOOD), LV_PART_INDICATOR);
  lv_obj_set_style_radius(meHpBar, 5, LV_PART_INDICATOR);

  meSpBar = lv_bar_create(scr_battle);
  lv_obj_set_size(meSpBar, 280, 10);
  lv_obj_set_pos(meSpBar, 93, 306);
  lv_obj_set_style_bg_color(meSpBar, lv_color_hex(C_PANEL), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(meSpBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(meSpBar, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(meSpBar, lv_color_hex(C_ACCENT), LV_PART_INDICATOR);
  lv_obj_set_style_radius(meSpBar, 5, LV_PART_INDICATOR);

  const char *names[3] = { "攻击\n剪刀", "防御\n石头", "技能\n布" };
  for (int i = 0; i < 3; i++) {
    battleBtn[i] = mkBtnBase(scr_battle, names[i], 110, 54, &digifont_small);
    lv_obj_set_pos(battleBtn[i], 78 + i * 100, 330);
    lv_obj_add_event_cb(battleBtn[i], onBattleAction, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)i);
  }

  btnBattleBack = mkButton(scr_battle, "返回", 140, 40, &digifont, ACT_HOME);
  lv_obj_align(btnBattleBack, LV_ALIGN_TOP_MID, 0, 338);
  lv_obj_add_flag(btnBattleBack, LV_OBJ_FLAG_HIDDEN);
}

void onBattleAction(lv_event_t *e) {
  uintptr_t act = (uintptr_t)lv_event_get_user_data(e);
  battlePlay(g_battle, *g_pet, (uint8_t)act);
  refreshBattle();
  refreshHome();
}

// ---------------------------------------------------------------- lock

void setPm(bool locked) {
  esp_pm_config_t pm = {};
  pm.max_freq_mhz = locked ? 80 : 240;
  pm.min_freq_mhz = 40;
  pm.light_sleep_enable = true;
  esp_err_t err = esp_pm_configure(&pm);
  if (err != ESP_OK) {
    ESP_LOGW("pet-ui", "esp_pm_configure: %s", esp_err_to_name(err));
  }
  if (g_disp) {
    lv_timer_t *rt = lv_display_get_refr_timer(g_disp);
    if (rt) lv_timer_set_period(rt, locked ? LOCK_REFR_PERIOD_MS : UI_REFR_PERIOD_MS);
  }
}

void updateLockTime() {
  char hm[8] = "--:--";
  clockGetHM(hm, sizeof(hm));
  static char last[8] = "";
  if (strcmp(hm, last) != 0) {
    strncpy(last, hm, sizeof(last) - 1);
    last[sizeof(last) - 1] = 0;
    lv_label_set_text(lblLockTime, hm);
  }
}

void updateLockBattery() {
  PowerBatt b = powerBattery();
  int pct = b.pct >= 0 ? b.pct : (b.usb ? 100 : -1);
  if (pct >= 0) lv_label_set_text_fmt(lblLockPct, "%d%%", pct);
  else lv_label_set_text(lblLockPct, "--");

  uint32_t col = b.usb ? C_GOOD : (pct >= 50 ? C_GOOD : (pct >= 20 ? C_WARN : C_BAD));
  int fw = pct < 0 ? 2 : pct * 56 / 100;
  if (fw < 2) fw = 2;
  lv_obj_set_width(lockBatFill, fw);
  lv_obj_set_style_bg_color(lockBatFill, lv_color_hex(col), 0);

  lv_label_set_text(lblLockChg, b.usb ? "充电中" : "");
  lv_obj_set_style_text_color(lblLockChg,
      lv_color_hex(b.charging ? C_GOOD : C_ACCENT), 0);
}

void updateLock() {
  if (!g_pet) return;
  lv_label_set_text(lblLockName, g_pet->name());
  lv_label_set_text_fmt(lblLockInfo, "Lv.%u  %s", (unsigned)g_pet->level(),
                        dgTypeName(g_pet->type()));
  updateLockTime();
  updateLockBattery();
}

void lockNow() {
  if (g_locked) return;
  g_locked = true;
  g_training = false;
  if (g_pet) g_pet->save();
  clockSave();
  bsp_display_brightness_set(g_bright);
  setPm(true);
  imuSetEnabled(false);
  if (g_animTimer) lv_timer_pause(g_animTimer);
  updateLock();
  g_lastLockBat = millis();
  lv_screen_load(scr_lock);
}

void unlock() {
  if (!g_locked) return;
  g_locked = false;
  bsp_display_brightness_set(g_bright);
  setPm(false);
  imuSetEnabled(true);
  if (g_animTimer) lv_timer_resume(g_animTimer);
  lv_screen_load(scr_home);
  refreshHome();
}

void refreshTimeSet() {
  int h = 12, m = 0;
  clockGetParts(&h, &m);
  lv_label_set_text_fmt(lblTimeSet, "%02d:%02d", h, m);
}

void openTimeSet() {
  refreshTimeSet();
  lv_screen_load(scr_time);
}

void buildTimeSet() {
  scr_time = mkScreen();
  lv_obj_t *t = mkLabel(scr_time, "校时", &digifont, C_TEXT);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 18);

  lblTimeSet = mkLabel(scr_time, "--:--", &digifont_big, C_ACCENT);
  lv_obj_set_width(lblTimeSet, 300);
  lv_obj_align(lblTimeSet, LV_ALIGN_TOP_MID, 0, 70);

  struct Item { const char *text; UiAct act; };
  const Item items[4] = {
    { "时-", ACT_TIME_HM }, { "时+", ACT_TIME_HP },
    { "分-", ACT_TIME_MM }, { "分+", ACT_TIME_MP },
  };
  for (int i = 0; i < 4; i++) {
    lv_obj_t *b = mkButton(scr_time, items[i].text, 90, 44, &digifont_small, items[i].act);
    lv_obj_set_pos(b, 35 + i * 102, 200);
  }

  lv_obj_t *back = mkButton(scr_time, "返回", 120, 40, &digifont_small, ACT_SYSTEM);
  lv_obj_align(back, LV_ALIGN_TOP_MID, 0, 330);
}

void buildLock() {
  scr_lock = mkScreen();

  lv_obj_t *t = mkLabel(scr_lock, "锁屏", &digifont_small, C_DIM);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 30);

  lblLockTime = mkLabel(scr_lock, "--:--", &digifont_big, C_ACCENT);
  lv_obj_set_width(lblLockTime, 300);
  lv_obj_align(lblLockTime, LV_ALIGN_TOP_MID, 0, 64);

  lv_obj_t *body = lv_obj_create(scr_lock);
  lv_obj_set_size(body, 64, 30);
  lv_obj_set_pos(body, 160, 160);
  lv_obj_set_style_radius(body, 6, 0);
  lv_obj_set_style_border_width(body, 2, 0);
  lv_obj_set_style_border_color(body, lv_color_hex(C_DIM), 0);
  lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(body, 0, 0);
  lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(body, LV_OBJ_FLAG_CLICKABLE);

  lockBatFill = lv_obj_create(body);
  lv_obj_set_size(lockBatFill, 2, 18);
  lv_obj_set_pos(lockBatFill, 4, 4);
  lv_obj_set_style_radius(lockBatFill, 3, 0);
  lv_obj_set_style_bg_color(lockBatFill, lv_color_hex(C_GOOD), 0);
  lv_obj_set_style_border_width(lockBatFill, 0, 0);
  lv_obj_remove_flag(lockBatFill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(lockBatFill, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *nub = lv_obj_create(scr_lock);
  lv_obj_set_size(nub, 4, 14);
  lv_obj_set_pos(nub, 225, 168);
  lv_obj_set_style_radius(nub, 2, 0);
  lv_obj_set_style_bg_color(nub, lv_color_hex(C_DIM), 0);
  lv_obj_set_style_border_width(nub, 0, 0);
  lv_obj_remove_flag(nub, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(nub, LV_OBJ_FLAG_CLICKABLE);

  lblLockPct = mkLabel(scr_lock, "--", &digifont_small, C_TEXT);
  lv_obj_set_pos(lblLockPct, 238, 164);
  lv_obj_set_width(lblLockPct, 70);
  lv_obj_set_style_text_align(lblLockPct, LV_TEXT_ALIGN_LEFT, 0);

  lblLockChg = mkLabel(scr_lock, "", &digifont_small, C_GOOD);
  lv_obj_set_width(lblLockChg, 300);
  lv_obj_align(lblLockChg, LV_ALIGN_TOP_MID, 0, 204);

  lblLockName = mkLabel(scr_lock, "", &digifont, C_TEXT);
  lv_obj_align(lblLockName, LV_ALIGN_CENTER, 0, 20);

  lblLockInfo = mkLabel(scr_lock, "", &digifont_small, C_DIM);
  lv_obj_align(lblLockInfo, LV_ALIGN_CENTER, 0, 60);

  lv_obj_t *hint = mkLabel(scr_lock, "短按唤醒  长按关机", &digifont_small, C_DIM);
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 110);
}

// ---------------------------------------------------------------- events

void onAction(lv_event_t *e) {
  UiAct act = (UiAct)(uintptr_t)lv_event_get_user_data(e);

  switch (act) {
    case ACT_EVOLVE: openEvolve(); return;
    case ACT_TRAIN: openTraining(); return;
    case ACT_BATTLE: openBattle(); return;
    case ACT_STATUS: openStatus(); return;
    case ACT_TYPES: lv_screen_load(scr_types); return;
    case ACT_SYSTEM: openSystem(); return;
    case ACT_HOME: goHome(); return;
    case ACT_TIMESET: openTimeSet(); return;
    case ACT_TIME_HM: clockAdjust(-1, 0); refreshTimeSet(); return;
    case ACT_TIME_HP: clockAdjust(1, 0); refreshTimeSet(); return;
    case ACT_TIME_MM: clockAdjust(0, -1); refreshTimeSet(); return;
    case ACT_TIME_MP: clockAdjust(0, 1); refreshTimeSet(); return;
    case ACT_RESET:
      if (g_resetConfirm) {
        g_pet->reset();
        g_resetConfirm = false;
        toast("已重置存档");
        goHome();
        return;
      }
      g_resetConfirm = true;
      lv_label_set_text(lblSysMsg, "再点一次 重置存档 确认");
      return;
    default:
      refreshHome();
      return;
  }
}

void onEvolveTarget(lv_event_t *e) {
  uint8_t target = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  g_pet->evolveTo(target);
  g_pet->save();
  toast("进化成功");
  goHome();
}

void onDevolveTarget(lv_event_t *e) {
  uint8_t target = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  g_pet->devolveTo(target);
  g_pet->save();
  toast("退化完成");
  goHome();
}

// ---------------------------------------------------------------- timers

void animTick(lv_timer_t *) {
  g_animCount++;
  if (!g_pet || g_locked) return;

  static const int8_t bob[8] = { 0, -1, -3, -4, -3, -1, 1, 0 };
  uint8_t act = g_pet->act(millis());
  bool excited = (act == DG_ACT_HAPPY || act == DG_ACT_TRAIN || act == DG_ACT_ATTACK);
  int dy = bob[g_animCount % 8];
  if (excited) dy -= (g_animCount % 2) ? 6 : 2;
  petAvatarBob(g_avatar, dy);
  petAvatarSetBlink(g_avatar, (g_animCount % 11) == 0);
}

void statusTick(lv_timer_t *) {
  if (!g_pet) return;
  uint32_t now = millis();

  bool changed = g_pet->update(now);
  if (changed && !g_locked) {
    refreshHome();
    if (lv_screen_active() == scr_status) refreshStatus();
  }

  if (now - g_lastSave >= 300000) {
    if (g_pet->dirty) g_pet->save();
    g_lastSave = now;
  }
  if (now - g_lastClockSave >= 600000) {
    g_lastClockSave = now;
    clockSave();
  }

  if (g_training) {
    int32_t left = (int32_t)(g_trainEnd - now);
    if (left <= 0) {
      finishTraining();
    } else {
      unsigned sec = (unsigned)((left + 999) / 1000);
      lv_label_set_text_fmt(lblTrainTime, "%u", sec);
    }
  }

  if (g_locked) {
    updateLockTime();
    if (now - g_lastLockBat >= 30000) {
      g_lastLockBat = now;
      updateLockBattery();
    }
  } else if (lv_screen_active() == scr_system && now % 5000 < 1000) {
    refreshSystemInfo();
  }
}

void buttonTick(lv_timer_t *) {
  uint32_t now = millis();
  uint8_t ev = powerPoll();

  if (ev & PEV_SCREEN) { powerOff(); return; }
  if (g_locked) {
    if (ev & (PEV_A | PEV_B)) {
      unlock();
      if (ev & PEV_A) openMenu();
    }
    return;
  }
  if (ev & PEV_B) { lockNow(); return; }
  if (ev & PEV_A) {
    if (lv_screen_active() == scr_home) openMenu();
    else goHome();
    return;
  }

  bool shook = imuShake();
  if (g_training) {
    if (shook) addTrainHit();
    return;
  }
  if (shook && lv_screen_active() == scr_home && g_pet) {
    g_pet->oneShot = DG_ACT_HAPPY;
    g_pet->oneShotUntil = now + 1500;
    refreshHome();
  }
}

}  // namespace

void ui_init(Digimon &pet) {
  g_pet = &pet;

  s_ui.begin("ui", false);
  g_bright = s_ui.getUChar("bright", 100);
  if (g_bright < 10) g_bright = 100;

  powerInit();
  imuInit();
  clockInit();

  g_disp = lv_display_get_default();
  ESP_LOGI("pet-ui", "display %dx%d",
           (int)lv_display_get_horizontal_resolution(g_disp),
           (int)lv_display_get_vertical_resolution(g_disp));

  buildHome();
  buildMenu();
  buildStatus();
  buildTypes();
  buildSystem();
  buildTimeSet();
  buildEvolve();
  buildTraining();
  buildBattle();
  buildLock();

  refreshHome();
  lv_screen_load(scr_home);
  applyBrightness();
  setPm(false);

  g_lastSave = millis();
  g_lastClockSave = millis();
  g_animTimer = lv_timer_create(animTick, 400, nullptr);
  lv_timer_create(statusTick, 1000, nullptr);
  lv_timer_create(buttonTick, 60, nullptr);

  ESP_LOGI("pet-ui", "mascota lista: %s", pet.name());
}
