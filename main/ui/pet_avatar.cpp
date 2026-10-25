#include "pet_avatar.h"

#include <string.h>

LV_FONT_DECLARE(digifont_pet);

const lv_image_dsc_t *g_petImage = nullptr;

namespace {

constexpr uint32_t C_TEXT = 0xE8F1FA;
constexpr uint32_t C_DIM  = 0x7E90A6;
constexpr uint32_t C_WARN = 0xFFA726;
constexpr uint32_t C_BAD  = 0xEF5350;

void utf8First(char *out, const char *s) {
  out[0] = 0;
  if (!s || !*s) return;
  unsigned char c = (unsigned char)s[0];
  int n = 1;
  if ((c & 0xE0) == 0xC0) n = 2;
  else if ((c & 0xF0) == 0xE0) n = 3;
  else if ((c & 0xF8) == 0xF0) n = 4;
  memcpy(out, s, (size_t)n);
  out[n] = 0;
}

}  // namespace

void petAvatarCreate(PetAvatar &a, lv_obj_t *parent, int size, int yOffset) {
  a.size = size;
  a.baseY = yOffset;

  a.root = lv_obj_create(parent);
  lv_obj_set_size(a.root, size, size);
  lv_obj_align(a.root, LV_ALIGN_CENTER, 0, yOffset);
  lv_obj_set_style_radius(a.root, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(a.root, 4, 0);
  lv_obj_set_style_border_color(a.root, lv_color_hex(C_TEXT), 0);
  lv_obj_set_style_bg_color(a.root, lv_color_hex(C_TEXT), 0);
  lv_obj_set_style_bg_opa(a.root, 36, 0);
  lv_obj_set_style_pad_all(a.root, 0, 0);
  lv_obj_remove_flag(a.root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(a.root, LV_OBJ_FLAG_CLICKABLE);

  a.image = lv_image_create(a.root);
  lv_obj_set_size(a.image, size - 18, size - 18);
  lv_obj_center(a.image);
  lv_obj_add_flag(a.image, LV_OBJ_FLAG_HIDDEN);

  a.glyph = lv_label_create(a.root);
  lv_label_set_text(a.glyph, "");
  lv_obj_set_style_text_font(a.glyph, &digifont_pet, 0);
  lv_obj_set_style_text_color(a.glyph, lv_color_hex(C_TEXT), 0);
  lv_obj_center(a.glyph);

  a.eyeH = size * 10 / 100;
  int eyeW = size * 10 / 100;
  int eyeY = -size * 38 / 100;
  int eyeX = size * 21 / 100;

  a.eyeL = lv_obj_create(a.root);
  lv_obj_set_size(a.eyeL, eyeW, a.eyeH);
  lv_obj_set_style_radius(a.eyeL, eyeW / 2, 0);
  lv_obj_set_style_bg_color(a.eyeL, lv_color_hex(C_TEXT), 0);
  lv_obj_set_style_border_width(a.eyeL, 0, 0);
  lv_obj_align(a.eyeL, LV_ALIGN_CENTER, -eyeX, eyeY);

  a.eyeR = lv_obj_create(a.root);
  lv_obj_set_size(a.eyeR, eyeW, a.eyeH);
  lv_obj_set_style_radius(a.eyeR, eyeW / 2, 0);
  lv_obj_set_style_bg_color(a.eyeR, lv_color_hex(C_TEXT), 0);
  lv_obj_set_style_border_width(a.eyeR, 0, 0);
  lv_obj_align(a.eyeR, LV_ALIGN_CENTER, eyeX, eyeY);

  int msize = size * 30 / 100;
  a.mouth = lv_arc_create(a.root);
  lv_obj_set_size(a.mouth, msize, msize);
  lv_obj_align(a.mouth, LV_ALIGN_CENTER, 0, size * 30 / 100);
  lv_obj_remove_flag(a.mouth, LV_OBJ_FLAG_CLICKABLE);
  lv_arc_set_bg_angles(a.mouth, 0, 360);
  lv_arc_set_angles(a.mouth, 25, 155);
  lv_obj_set_style_arc_width(a.mouth, 5, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(a.mouth, lv_color_hex(C_TEXT), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(a.mouth, true, LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(a.mouth, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(a.mouth, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_width(a.mouth, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(a.mouth, 0, LV_PART_KNOB);
}

void petAvatarSetSpecies(PetAvatar &a, const char *name, uint32_t accent) {
  if (!a.root) return;
  char ch[8];
  utf8First(ch, name);
  lv_label_set_text(a.glyph, ch);
  lv_obj_set_style_border_color(a.root, lv_color_hex(accent), 0);
  lv_obj_set_style_bg_color(a.root, lv_color_hex(accent), 0);
}

void petAvatarSetMood(PetAvatar &a, int mood, bool excited) {
  if (!a.root) return;
  uint32_t mc = excited ? C_TEXT : (mood >= 3 ? C_TEXT : (mood >= 1 ? C_WARN : C_BAD));
  lv_obj_set_style_arc_color(a.mouth, lv_color_hex(mc), LV_PART_INDICATOR);
  if (excited) lv_arc_set_angles(a.mouth, 20, 160);
  else if (mood >= 3) lv_arc_set_angles(a.mouth, 25, 155);
  else if (mood == 2) lv_arc_set_angles(a.mouth, 55, 125);
  else lv_arc_set_angles(a.mouth, 205, 335);
  lv_obj_set_style_bg_color(a.eyeL, lv_color_hex(mood >= 1 ? C_TEXT : C_DIM), 0);
  lv_obj_set_style_bg_color(a.eyeR, lv_color_hex(mood >= 1 ? C_TEXT : C_DIM), 0);
}

void petAvatarSetBlink(PetAvatar &a, bool blink) {
  if (!a.eyeL) return;
  int h = blink ? 4 : a.eyeH;
  lv_obj_set_height(a.eyeL, h);
  lv_obj_set_height(a.eyeR, h);
}

void petAvatarBob(PetAvatar &a, int dy) {
  if (!a.root) return;
  lv_obj_align(a.root, LV_ALIGN_CENTER, 0, a.baseY + dy);
}

void petAvatarSetImage(PetAvatar &a, const lv_image_dsc_t *dsc) {
  if (!a.root) return;
  if (dsc) {
    lv_image_set_src(a.image, dsc);
    lv_obj_remove_flag(a.image, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a.glyph, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a.eyeL, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a.eyeR, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a.mouth, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(a.image, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(a.glyph, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(a.eyeL, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(a.eyeR, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(a.mouth, LV_OBJ_FLAG_HIDDEN);
  }
}
