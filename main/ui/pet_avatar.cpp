#include "pet_avatar.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "misc/cache/instance/lv_image_cache.h"

#include <stdlib.h>
#include <string.h>

const lv_image_dsc_t *g_petImage = nullptr;

namespace {

const char *TAG = "avatar";
constexpr uint32_t C_DETAIL = 0xE8F1FA;

// Formas normalizadas 0..100. Cada par es x,y. Las siluetas son polilineas.
const int16_t EGG[] = {
  50,6, 64,10, 76,22, 82,40, 82,60, 76,78, 64,90, 50,94,
  36,90, 24,78, 18,60, 18,40, 24,22, 36,10, 50,6,
};
const int16_t EGG_ZIG[] = { 36,32, 50,42, 36,52, 50,62 };

const int16_t BABY_BODY[] = {
  50,16, 64,20, 72,36, 70,56, 58,70, 42,70, 30,56, 28,36, 36,20, 50,16,
};
const int16_t BABY_LEG1[] = { 42,70, 38,86, 48,86 };
const int16_t BABY_LEG2[] = { 58,70, 62,86, 52,86 };
const int16_t BABY_TAIL[] = { 28,52, 14,60, 18,74, 30,66 };

const int16_t ROOKIE_HEAD[] = {
  38,18, 54,14, 66,24, 64,38, 52,46, 38,42, 32,30, 38,18,
};
const int16_t ROOKIE_BODY[] = { 46,46, 36,62, 40,82, 58,82, 62,62, 54,46 };
const int16_t ROOKIE_ARM1[] = { 42,52, 26,60 };
const int16_t ROOKIE_ARM2[] = { 58,52, 74,60 };
const int16_t ROOKIE_LEG1[] = { 44,82, 40,94 };
const int16_t ROOKIE_LEG2[] = { 56,82, 60,94 };
const int16_t ROOKIE_TAIL[] = { 36,68, 16,72, 20,58 };
const int16_t ROOKIE_HORN1[] = { 36,18, 30,8 };
const int16_t ROOKIE_HORN2[] = { 60,20, 66,10 };

const int16_t CHAMP_OUTLINE[] = {
  50,8, 64,14, 70,30, 66,48, 76,66, 70,86, 54,94, 46,94,
  30,86, 24,66, 34,48, 30,30, 36,14, 50,8,
};
const int16_t CHAMP_HORN1[] = { 38,14, 28,2 };
const int16_t CHAMP_HORN2[] = { 62,14, 72,2 };
const int16_t CHAMP_ARM1[] = { 32,52, 14,58 };
const int16_t CHAMP_ARM2[] = { 68,52, 86,58 };
const int16_t CHAMP_LEG1[] = { 44,94, 36,98 };
const int16_t CHAMP_LEG2[] = { 56,94, 64,98 };
const int16_t CHAMP_TAIL[] = { 30,70, 10,76, 16,60 };
const int16_t CHAMP_CHEST[] = { 40,50, 60,50 };

const int16_t ULT_OUTLINE[] = {
  50,6, 68,14, 78,30, 72,50, 84,72, 70,92, 50,98, 30,92,
  16,72, 28,50, 22,30, 32,14, 50,6,
};
const int16_t ULT_SHOULDER1[] = { 32,42, 14,36, 18,52 };
const int16_t ULT_SHOULDER2[] = { 68,42, 86,36, 82,52 };
const int16_t ULT_HORN1[] = { 40,14, 34,2 };
const int16_t ULT_HORN2[] = { 60,14, 66,2 };
const int16_t ULT_CHEST1[] = { 40,54, 60,54 };
const int16_t ULT_CHEST2[] = { 50,54, 50,84 };
const int16_t ULT_LEG1[] = { 42,98, 34,96 };
const int16_t ULT_LEG2[] = { 58,98, 66,96 };

const int16_t MEGA_BODY[] = {
  50,14, 62,18, 68,34, 62,50, 72,74, 60,92, 40,92, 28,74,
  38,50, 32,34, 38,18, 50,14,
};
const int16_t MEGA_WING1[] = { 34,32, 8,16, 16,42, 4,48, 30,52 };
const int16_t MEGA_WING2[] = { 66,32, 92,16, 84,42, 96,48, 70,52 };
const int16_t MEGA_CROWN[] = { 40,14, 34,2, 44,8, 50,0, 56,8, 66,2, 60,14 };
const int16_t MEGA_LEG1[] = { 42,92, 34,98 };
const int16_t MEGA_LEG2[] = { 58,92, 66,98 };
const int16_t MEGA_TAIL[] = { 28,68, 12,78, 18,58 };

uint32_t argb(uint32_t rgb) {
  uint32_t r = (rgb >> 16) & 0xFF;
  uint32_t g = (rgb >> 8) & 0xFF;
  uint32_t b = rgb & 0xFF;
  return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

void putPx(PetAvatar &a, int x, int y, uint32_t c) {
  if (!a.buf || x < 0 || y < 0 || x >= a.size || y >= a.size) return;
  a.buf[y * a.size + x] = c;
}

int scaleX(PetAvatar &a, int nx) {
  return 6 + nx * (a.size - 12) / 100;
}

void line(PetAvatar &a, int x0, int y0, int x1, int y1, uint32_t c, int t) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    for (int oy = -t / 2; oy <= t / 2; oy++) {
      for (int ox = -t / 2; ox <= t / 2; ox++) putPx(a, x0 + ox, y0 + oy, c);
    }
    if (x0 == x1 && y0 == y1) break;
    int e2 = err * 2;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void poly(PetAvatar &a, const int16_t *pts, int n, uint32_t c, int t) {
  for (int i = 0; i + 3 < n * 2; i += 2) {
    line(a, scaleX(a, pts[i]), scaleX(a, pts[i + 1]),
         scaleX(a, pts[i + 2]), scaleX(a, pts[i + 3]), c, t);
  }
}

void block(PetAvatar &a, int nx, int ny, uint32_t c, int px) {
  int x = scaleX(a, nx), y = scaleX(a, ny);
  for (int oy = 0; oy < px; oy++)
    for (int ox = 0; ox < px; ox++) putPx(a, x + ox, y + oy, c);
}

void redraw(PetAvatar &a) {
  if (!a.buf || !a.image) return;
  memset(a.buf, 0, (size_t)a.size * a.size * 4);
  uint32_t c = argb(a.accent);
  uint32_t d = argb(C_DETAIL);

  switch (a.stage) {
    case 0:
      poly(a, EGG, sizeof(EGG) / 4, c, 4);
      poly(a, EGG_ZIG, sizeof(EGG_ZIG) / 4, d, 3);
      block(a, 49, 48, d, 6);
      break;
    case 1:
      poly(a, BABY_BODY, sizeof(BABY_BODY) / 4, c, 4);
      poly(a, BABY_LEG1, sizeof(BABY_LEG1) / 4, c, 4);
      poly(a, BABY_LEG2, sizeof(BABY_LEG2) / 4, c, 4);
      poly(a, BABY_TAIL, sizeof(BABY_TAIL) / 4, d, 3);
      block(a, 48, 42, d, 6);
      break;
    case 2:
      poly(a, ROOKIE_HEAD, sizeof(ROOKIE_HEAD) / 4, c, 4);
      poly(a, ROOKIE_BODY, sizeof(ROOKIE_BODY) / 4, c, 4);
      poly(a, ROOKIE_ARM1, sizeof(ROOKIE_ARM1) / 4, c, 3);
      poly(a, ROOKIE_ARM2, sizeof(ROOKIE_ARM2) / 4, c, 3);
      poly(a, ROOKIE_LEG1, sizeof(ROOKIE_LEG1) / 4, c, 4);
      poly(a, ROOKIE_LEG2, sizeof(ROOKIE_LEG2) / 4, c, 4);
      poly(a, ROOKIE_TAIL, sizeof(ROOKIE_TAIL) / 4, d, 3);
      poly(a, ROOKIE_HORN1, sizeof(ROOKIE_HORN1) / 4, d, 3);
      poly(a, ROOKIE_HORN2, sizeof(ROOKIE_HORN2) / 4, d, 3);
      block(a, 48, 30, d, 6);
      break;
    case 3:
      poly(a, CHAMP_OUTLINE, sizeof(CHAMP_OUTLINE) / 4, c, 4);
      poly(a, CHAMP_HORN1, sizeof(CHAMP_HORN1) / 4, d, 3);
      poly(a, CHAMP_HORN2, sizeof(CHAMP_HORN2) / 4, d, 3);
      poly(a, CHAMP_ARM1, sizeof(CHAMP_ARM1) / 4, c, 4);
      poly(a, CHAMP_ARM2, sizeof(CHAMP_ARM2) / 4, c, 4);
      poly(a, CHAMP_LEG1, sizeof(CHAMP_LEG1) / 4, c, 4);
      poly(a, CHAMP_LEG2, sizeof(CHAMP_LEG2) / 4, c, 4);
      poly(a, CHAMP_TAIL, sizeof(CHAMP_TAIL) / 4, d, 3);
      poly(a, CHAMP_CHEST, sizeof(CHAMP_CHEST) / 4, d, 2);
      block(a, 48, 24, d, 6);
      break;
    case 4:
      poly(a, ULT_OUTLINE, sizeof(ULT_OUTLINE) / 4, c, 4);
      poly(a, ULT_SHOULDER1, sizeof(ULT_SHOULDER1) / 4, c, 4);
      poly(a, ULT_SHOULDER2, sizeof(ULT_SHOULDER2) / 4, c, 4);
      poly(a, ULT_HORN1, sizeof(ULT_HORN1) / 4, d, 3);
      poly(a, ULT_HORN2, sizeof(ULT_HORN2) / 4, d, 3);
      poly(a, ULT_CHEST1, sizeof(ULT_CHEST1) / 4, d, 2);
      poly(a, ULT_CHEST2, sizeof(ULT_CHEST2) / 4, d, 2);
      poly(a, ULT_LEG1, sizeof(ULT_LEG1) / 4, c, 4);
      poly(a, ULT_LEG2, sizeof(ULT_LEG2) / 4, c, 4);
      block(a, 48, 28, d, 6);
      break;
    default:
      poly(a, MEGA_BODY, sizeof(MEGA_BODY) / 4, c, 4);
      poly(a, MEGA_WING1, sizeof(MEGA_WING1) / 4, d, 3);
      poly(a, MEGA_WING2, sizeof(MEGA_WING2) / 4, d, 3);
      poly(a, MEGA_CROWN, sizeof(MEGA_CROWN) / 4, d, 3);
      poly(a, MEGA_LEG1, sizeof(MEGA_LEG1) / 4, c, 4);
      poly(a, MEGA_LEG2, sizeof(MEGA_LEG2) / 4, c, 4);
      poly(a, MEGA_TAIL, sizeof(MEGA_TAIL) / 4, d, 3);
      block(a, 48, 26, d, 6);
      break;
  }

  lv_image_cache_drop(&a.dsc);
  a.dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  a.dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
  a.dsc.header.w = (uint32_t)a.size;
  a.dsc.header.h = (uint32_t)a.size;
  a.dsc.header.stride = (uint32_t)a.size * 4;
  a.dsc.data_size = (uint32_t)a.size * a.size * 4;
  a.dsc.data = (const uint8_t *)a.buf;
  if (!a.realImage) lv_image_set_src(a.image, &a.dsc);
  lv_obj_invalidate(a.image);
}

}  // namespace

void petAvatarCreate(PetAvatar &a, lv_obj_t *parent, int size, int yOffset) {
  a.size = size;
  a.baseY = yOffset;

  a.root = lv_obj_create(parent);
  lv_obj_set_size(a.root, size, size);
  lv_obj_align(a.root, LV_ALIGN_CENTER, 0, yOffset);
  lv_obj_set_style_radius(a.root, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(a.root, 3, 0);
  lv_obj_set_style_border_color(a.root, lv_color_hex(C_DETAIL), 0);
  lv_obj_set_style_bg_color(a.root, lv_color_hex(C_DETAIL), 0);
  lv_obj_set_style_bg_opa(a.root, 18, 0);
  lv_obj_set_style_pad_all(a.root, 0, 0);
  lv_obj_remove_flag(a.root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(a.root, LV_OBJ_FLAG_CLICKABLE);

  a.buf = (uint32_t *)heap_caps_malloc((size_t)size * size * 4,
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!a.buf) {
    ESP_LOGE(TAG, "sin buffer de avatar");
    return;
  }
  memset(a.buf, 0, (size_t)size * size * 4);
  a.dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  a.dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
  a.dsc.header.w = (uint32_t)size;
  a.dsc.header.h = (uint32_t)size;
  a.dsc.header.stride = (uint32_t)size * 4;
  a.dsc.data_size = (uint32_t)size * size * 4;
  a.dsc.data = (const uint8_t *)a.buf;

  a.image = lv_image_create(a.root);
  lv_obj_set_size(a.image, size, size);
  lv_obj_center(a.image);
  lv_obj_remove_flag(a.image, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(a.image, LV_OBJ_FLAG_SCROLLABLE);
}

void petAvatarSetSpecies(PetAvatar &a, uint32_t accent, uint8_t stage) {
  if (!a.root) return;
  if (a.stage == stage && a.accent == accent) return;
  a.stage = stage;
  a.accent = accent;
  lv_obj_set_style_border_color(a.root, lv_color_hex(accent), 0);
  lv_obj_set_style_bg_color(a.root, lv_color_hex(accent), 0);
  if (!a.realImage) redraw(a);
}

void petAvatarSetMood(PetAvatar &a, int mood, bool excited, bool sleeping) {
  if (!a.image) return;
  lv_opa_t opa;
  if (sleeping) opa = 80;
  else if (excited) opa = 255;
  else if (mood >= 3) opa = 235;
  else if (mood == 2) opa = 200;
  else opa = 150;
  lv_obj_set_style_opa(a.image, opa, 0);
}

void petAvatarBob(PetAvatar &a, int dy) {
  if (!a.root) return;
  lv_obj_align(a.root, LV_ALIGN_CENTER, 0, a.baseY + dy);
}

void petAvatarSetImage(PetAvatar &a, const lv_image_dsc_t *dsc) {
  if (!a.image) return;
  if (dsc) {
    a.realImage = true;
    lv_image_set_src(a.image, dsc);
  } else if (a.realImage) {
    a.realImage = false;
    redraw(a);
  }
}
