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

enum Body : uint8_t {
  B_EGG, B_BLOB, B_BIPED, B_DINO, B_QUAD, B_BIRD, B_INSECT,
  B_AQUA, B_PLANT, B_ANGEL, B_DEMON, B_MECH, B_KNIGHT,
};

enum : uint16_t {
  F_HORN      = 1 << 0,
  F_EARS      = 1 << 1,
  F_TAIL      = 1 << 2,
  F_WINGS     = 1 << 3,
  F_HALO      = 1 << 4,
  F_CREST     = 1 << 5,
  F_SPIKES    = 1 << 6,
  F_CANNON    = 1 << 7,
  F_CROWN     = 1 << 8,
  F_CAPE      = 1 << 9,
  F_SWORD     = 1 << 10,
  F_ANTENNAE  = 1 << 11,
  F_FIN       = 1 << 12,
  F_LEAF      = 1 << 13,
  F_ARMOR     = 1 << 14,
  F_CLAWS     = 1 << 15,
};

struct ShapeRow { uint8_t body; uint16_t feat; uint8_t scale; };

// Una fila por forma del config (41 formas).
const ShapeRow SHAPES[] = {
  { B_EGG,    0, 80 },
  { B_BLOB,   0, 70 },
  { B_BLOB,   F_EARS | F_TAIL, 74 },
  { B_DINO,   F_HORN | F_TAIL, 80 },
  { B_BIPED,  F_HORN | F_TAIL, 80 },
  { B_BLOB,   F_EARS | F_WINGS, 72 },
  { B_BIRD,   F_CREST | F_WINGS, 78 },
  { B_INSECT, F_ANTENNAE | F_WINGS, 76 },
  { B_AQUA,   F_HORN | F_FIN, 78 },
  { B_PLANT,  F_LEAF, 78 },
  { B_QUAD,   F_EARS, 72 },
  { B_DINO,   F_HORN | F_SPIKES | F_TAIL, 90 },
  { B_QUAD,   F_HORN | F_TAIL, 88 },
  { B_ANGEL,  F_HALO | F_WINGS, 86 },
  { B_BIRD,   F_CREST | F_WINGS | F_TAIL, 90 },
  { B_INSECT, F_HORN | F_WINGS, 94 },
  { B_AQUA,   F_HORN | F_FIN, 90 },
  { B_PLANT,  F_SPIKES, 90 },
  { B_BIPED,  F_EARS | F_TAIL, 84 },
  { B_DEMON,  F_HORN | F_WINGS | F_CAPE, 94 },
  { B_MECH,   F_CANNON | F_WINGS | F_TAIL | F_ARMOR, 100 },
  { B_BIPED,  F_EARS | F_TAIL | F_SPIKES | F_CLAWS, 100 },
  { B_ANGEL,  F_HALO | F_WINGS | F_ARMOR, 100 },
  { B_BIRD,   F_CREST | F_WINGS | F_CLAWS, 100 },
  { B_INSECT, F_HORN | F_WINGS | F_CANNON, 100 },
  { B_AQUA,   F_HORN | F_FIN | F_ARMOR, 100 },
  { B_PLANT,  F_LEAF | F_WINGS, 100 },
  { B_ANGEL,  F_HALO | F_WINGS | F_CREST, 100 },
  { B_DINO,   F_HORN | F_SPIKES | F_TAIL | F_CLAWS, 100 },
  { B_DEMON,  F_HORN | F_WINGS | F_CAPE | F_CLAWS, 100 },
  { B_MECH,   F_HORN | F_CANNON | F_WINGS | F_TAIL | F_ARMOR, 100 },
  { B_MECH,   F_HORN | F_CANNON | F_TAIL | F_ARMOR, 100 },
  { B_ANGEL,  F_HALO | F_WINGS | F_ARMOR | F_CROWN, 100 },
  { B_BIRD,   F_CREST | F_WINGS | F_CROWN, 100 },
  { B_INSECT, F_HORN | F_WINGS | F_CANNON | F_ARMOR, 100 },
  { B_AQUA,   F_HORN | F_FIN | F_TAIL, 100 },
  { B_PLANT,  F_LEAF | F_SPIKES | F_CROWN, 100 },
  { B_ANGEL,  F_HALO | F_WINGS | F_CROWN, 100 },
  { B_DEMON,  F_HORN | F_WINGS | F_CAPE | F_CROWN, 100 },
  { B_KNIGHT, F_HORN | F_WINGS | F_CANNON | F_SWORD | F_CAPE, 100 },
  { B_DINO,   F_HORN | F_WINGS | F_CANNON | F_TAIL | F_ARMOR, 100 },
};

#define NPTS(a) ((int)(sizeof(a) / sizeof((a)[0]) / 2))

const int16_t EGG[] = {
  50,6, 64,10, 76,22, 82,40, 82,60, 76,78, 64,90, 50,94,
  36,90, 24,78, 18,60, 18,40, 24,22, 36,10, 50,6,
};
const int16_t EGG_ZIG[] = { 36,32, 50,42, 36,52, 50,62 };

const int16_t BLOB_BODY[] = {
  50,16, 64,20, 72,36, 70,56, 58,70, 42,70, 30,56, 28,36, 36,20, 50,16,
};
const int16_t BLOB_LEG1[] = { 42,70, 38,86, 48,86 };
const int16_t BLOB_LEG2[] = { 58,70, 62,86, 52,86 };

const int16_t BIPED_HEAD[] = {
  38,18, 54,14, 66,24, 64,38, 52,46, 38,42, 32,30, 38,18,
};
const int16_t BIPED_BODY[] = { 46,46, 36,62, 40,82, 58,82, 62,62, 54,46 };
const int16_t BIPED_ARM1[] = { 42,52, 26,60 };
const int16_t BIPED_ARM2[] = { 58,52, 74,60 };
const int16_t BIPED_LEG1[] = { 44,82, 40,94 };
const int16_t BIPED_LEG2[] = { 56,82, 60,94 };

const int16_t DINO_BODY[] = {
  50,10, 64,16, 72,32, 66,48, 78,66, 70,86, 54,94, 46,94,
  30,86, 22,66, 34,48, 28,32, 36,16, 50,10,
};
const int16_t DINO_ARM1[] = { 34,54, 18,62 };
const int16_t DINO_ARM2[] = { 66,54, 82,62 };
const int16_t DINO_LEG1[] = { 44,94, 36,98 };
const int16_t DINO_LEG2[] = { 56,94, 64,98 };

const int16_t QUAD_BODY[] = {
  30,50, 40,40, 60,40, 70,50, 68,64, 50,70, 32,64, 30,50,
};
const int16_t QUAD_HEAD[] = { 35,38, 50,30, 65,38, 60,52, 40,52, 35,38 };
const int16_t QUAD_LEG1[] = { 34,64, 30,82, 38,82 };
const int16_t QUAD_LEG2[] = { 44,68, 42,84, 50,84 };
const int16_t QUAD_LEG3[] = { 56,68, 58,84, 66,84 };
const int16_t QUAD_LEG4[] = { 66,64, 70,82, 62,82 };

const int16_t BIRD_BODY[] = {
  50,30, 62,36, 66,52, 58,66, 50,70, 42,66, 34,52, 38,36, 50,30,
};
const int16_t BIRD_BEAK[] = { 54,40, 64,44, 54,46 };
const int16_t BIRD_LEG1[] = { 44,70, 42,84, 48,84 };
const int16_t BIRD_LEG2[] = { 56,70, 58,84, 52,84 };

const int16_t INSECT_BODY[] = {
  50,26, 64,30, 70,44, 64,58, 50,66, 36,58, 30,44, 36,30, 50,26,
};
const int16_t INSECT_SEG1[] = { 36,44, 64,44 };
const int16_t INSECT_SEG2[] = { 38,54, 62,54 };
const int16_t INSECT_LEG1[] = { 38,58, 28,68 };
const int16_t INSECT_LEG2[] = { 62,58, 72,68 };
const int16_t INSECT_LEG3[] = { 40,62, 34,76 };
const int16_t INSECT_LEG4[] = { 60,62, 66,76 };

const int16_t AQUA_BODY[] = {
  50,30, 64,36, 72,50, 68,64, 56,72, 44,72, 32,64, 28,50, 36,36, 50,30,
};
const int16_t AQUA_TAIL[] = { 50,72, 42,84, 50,90, 58,84, 50,72 };

const int16_t PLANT_BODY[] = {
  50,26, 64,30, 70,44, 66,60, 54,70, 46,70, 34,60, 30,44, 36,30, 50,26,
};
const int16_t PLANT_STEM[] = { 50,70, 50,84 };

const int16_t MECH_BODY[] = {
  50,20, 64,24, 70,36, 66,50, 76,62, 70,80, 56,86, 44,86,
  30,80, 24,62, 34,50, 30,36, 36,24, 50,20,
};
const int16_t MECH_ARMOR1[] = { 32,40, 20,36, 24,50, 34,50 };
const int16_t MECH_ARMOR2[] = { 68,40, 80,36, 76,50, 66,50 };
const int16_t MECH_LEG1[] = { 44,86, 40,94 };
const int16_t MECH_LEG2[] = { 56,86, 60,94 };

const int16_t WING_BIRD_L[] = { 34,36, 10,24, 18,44, 6,50, 32,52 };
const int16_t WING_BIRD_R[] = { 66,36, 90,24, 82,44, 94,50, 68,52 };
const int16_t WING_ANGEL_L[] = { 34,38, 12,16, 18,34, 6,40, 20,48, 34,50 };
const int16_t WING_ANGEL_R[] = { 66,38, 88,16, 82,34, 94,40, 80,48, 66,50 };
const int16_t WING_BAT_L[] = { 34,38, 12,28, 18,46, 8,54, 24,50, 34,52 };
const int16_t WING_BAT_R[] = { 66,38, 88,28, 82,46, 92,54, 76,50, 66,52 };
const int16_t WING_MECH_L[] = { 32,34, 12,24, 18,42, 8,48, 30,50 };
const int16_t WING_MECH_R[] = { 68,34, 88,24, 82,42, 92,48, 70,50 };
const int16_t WING_SMALL_L[] = { 34,40, 18,30, 24,44, 34,48 };
const int16_t WING_SMALL_R[] = { 66,40, 82,30, 76,44, 66,48 };
const int16_t HALO[] = {
  40,22, 46,18, 54,18, 60,22, 56,26, 44,26, 40,22,
};

uint32_t argb(uint32_t rgb) {
  uint32_t r = (rgb >> 16) & 0xFF;
  uint32_t g = (rgb >> 8) & 0xFF;
  uint32_t b = rgb & 0xFF;
  return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

int coord(PetAvatar &a, int n) {
  float s = a.scalePct / 100.0f;
  float v = 50.0f + (n - 50.0f) * s;
  return 6 + (int)(v * (a.size - 12) / 100.0f);
}

void putPx(PetAvatar &a, int x, int y, uint32_t c) {
  if (!a.buf || x < 0 || y < 0 || x >= a.size || y >= a.size) return;
  a.buf[y * a.size + x] = c;
}

void line(PetAvatar &a, int x0, int y0, int x1, int y1, uint32_t c, int t) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    for (int oy = -t / 2; oy <= t / 2; oy++)
      for (int ox = -t / 2; ox <= t / 2; ox++) putPx(a, x0 + ox, y0 + oy, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = err * 2;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void poly(PetAvatar &a, const int16_t *pts, int n, uint32_t c, int t) {
  for (int i = 0; i + 3 < n * 2; i += 2) {
    line(a, coord(a, pts[i]), coord(a, pts[i + 1]),
         coord(a, pts[i + 2]), coord(a, pts[i + 3]), c, t);
  }
}

void box(PetAvatar &a, int nx, int ny, int w, int h, uint32_t c) {
  int x = coord(a, nx) - w / 2;
  int y = coord(a, ny) - h / 2;
  for (int oy = 0; oy < h; oy++)
    for (int ox = 0; ox < w; ox++) putPx(a, x + ox, y + oy, c);
}

void faceFor(uint8_t body, int &fx, int &fy, int &spread) {
  fx = 50; fy = 40; spread = 14;
  switch (body) {
    case B_EGG:    fy = 52; spread = 12; break;
    case B_BLOB:   fy = 44; spread = 14; break;
    case B_BIPED:  fy = 30; spread = 13; break;
    case B_DINO:   fy = 32; spread = 15; break;
    case B_QUAD:   fy = 44; spread = 15; break;
    case B_BIRD:   fy = 38; spread = 11; break;
    case B_INSECT: fy = 36; spread = 17; break;
    case B_AQUA:   fy = 44; spread = 14; break;
    case B_PLANT:  fy = 40; spread = 13; break;
    default:       fy = 32; spread = 13; break;
  }
}

void drawFace(PetAvatar &a, int fx, int fy, int spread, int expr, uint32_t d) {
  int ex = spread / 2;
  if (expr == 1) {
    box(a, fx - ex, fy + 2, 8, 2, d);
    box(a, fx + ex, fy + 2, 8, 2, d);
    box(a, fx - 8, fy + 8, 4, 2, d);
    box(a, fx - 4, fy + 11, 4, 2, d);
    box(a, fx, fy + 13, 4, 2, d);
    box(a, fx + 4, fy + 11, 4, 2, d);
    box(a, fx + 8, fy + 8, 4, 2, d);
  } else if (expr == 2) {
    box(a, fx - ex, fy, 8, 2, d);
    box(a, fx + ex, fy, 8, 2, d);
    box(a, fx, fy + 10, 5, 2, d);
  } else if (expr == 3) {
    box(a, fx - ex, fy, 5, 6, d);
    box(a, fx + ex, fy, 5, 6, d);
    line(a, coord(a, fx - 14), coord(a, fy - 6), coord(a, fx - 5), coord(a, fy - 2), d, 2);
    line(a, coord(a, fx + 14), coord(a, fy - 6), coord(a, fx + 5), coord(a, fy - 2), d, 2);
    box(a, fx - 8, fy + 12, 4, 2, d);
    box(a, fx - 4, fy + 9, 4, 2, d);
    box(a, fx, fy + 8, 4, 2, d);
    box(a, fx + 4, fy + 9, 4, 2, d);
    box(a, fx + 8, fy + 12, 4, 2, d);
  } else {
    box(a, fx - ex, fy, 5, 6, d);
    box(a, fx + ex, fy, 5, 6, d);
    box(a, fx, fy + 10, 7, 2, d);
  }
}

void drawBody(PetAvatar &a, uint8_t body, uint32_t c, uint32_t d) {
  switch (body) {
    case B_EGG:
      poly(a, EGG, NPTS(EGG), c, 4);
      poly(a, EGG_ZIG, NPTS(EGG_ZIG), d, 3);
      box(a, 49, 48, 6, 6, d);
      break;
    case B_BLOB:
      poly(a, BLOB_BODY, NPTS(BLOB_BODY), c, 4);
      poly(a, BLOB_LEG1, NPTS(BLOB_LEG1), c, 4);
      poly(a, BLOB_LEG2, NPTS(BLOB_LEG2), c, 4);
      break;
    case B_BIPED:
    case B_ANGEL:
    case B_DEMON:
    case B_KNIGHT:
      poly(a, BIPED_HEAD, NPTS(BIPED_HEAD), c, 4);
      poly(a, BIPED_BODY, NPTS(BIPED_BODY), c, 4);
      poly(a, BIPED_ARM1, NPTS(BIPED_ARM1), c, 3);
      poly(a, BIPED_ARM2, NPTS(BIPED_ARM2), c, 3);
      poly(a, BIPED_LEG1, NPTS(BIPED_LEG1), c, 4);
      poly(a, BIPED_LEG2, NPTS(BIPED_LEG2), c, 4);
      break;
    case B_DINO:
      poly(a, DINO_BODY, NPTS(DINO_BODY), c, 4);
      poly(a, DINO_ARM1, NPTS(DINO_ARM1), c, 3);
      poly(a, DINO_ARM2, NPTS(DINO_ARM2), c, 3);
      poly(a, DINO_LEG1, NPTS(DINO_LEG1), c, 4);
      poly(a, DINO_LEG2, NPTS(DINO_LEG2), c, 4);
      break;
    case B_QUAD:
      poly(a, QUAD_BODY, NPTS(QUAD_BODY), c, 4);
      poly(a, QUAD_HEAD, NPTS(QUAD_HEAD), c, 4);
      poly(a, QUAD_LEG1, NPTS(QUAD_LEG1), c, 4);
      poly(a, QUAD_LEG2, NPTS(QUAD_LEG2), c, 4);
      poly(a, QUAD_LEG3, NPTS(QUAD_LEG3), c, 4);
      poly(a, QUAD_LEG4, NPTS(QUAD_LEG4), c, 4);
      break;
    case B_BIRD:
      poly(a, BIRD_BODY, NPTS(BIRD_BODY), c, 4);
      poly(a, BIRD_BEAK, NPTS(BIRD_BEAK), d, 3);
      poly(a, BIRD_LEG1, NPTS(BIRD_LEG1), c, 4);
      poly(a, BIRD_LEG2, NPTS(BIRD_LEG2), c, 4);
      break;
    case B_INSECT:
      poly(a, INSECT_BODY, NPTS(INSECT_BODY), c, 4);
      poly(a, INSECT_SEG1, NPTS(INSECT_SEG1), d, 2);
      poly(a, INSECT_SEG2, NPTS(INSECT_SEG2), d, 2);
      poly(a, INSECT_LEG1, NPTS(INSECT_LEG1), c, 3);
      poly(a, INSECT_LEG2, NPTS(INSECT_LEG2), c, 3);
      poly(a, INSECT_LEG3, NPTS(INSECT_LEG3), c, 3);
      poly(a, INSECT_LEG4, NPTS(INSECT_LEG4), c, 3);
      break;
    case B_AQUA:
      poly(a, AQUA_BODY, NPTS(AQUA_BODY), c, 4);
      poly(a, AQUA_TAIL, NPTS(AQUA_TAIL), c, 4);
      break;
    case B_PLANT:
      poly(a, PLANT_BODY, NPTS(PLANT_BODY), c, 4);
      poly(a, PLANT_STEM, NPTS(PLANT_STEM), c, 4);
      break;
    case B_MECH:
      poly(a, MECH_BODY, NPTS(MECH_BODY), c, 4);
      poly(a, MECH_ARMOR1, NPTS(MECH_ARMOR1), d, 3);
      poly(a, MECH_ARMOR2, NPTS(MECH_ARMOR2), d, 3);
      poly(a, MECH_LEG1, NPTS(MECH_LEG1), c, 4);
      poly(a, MECH_LEG2, NPTS(MECH_LEG2), c, 4);
      break;
  }
}

void drawBack(PetAvatar &a, uint8_t body, uint16_t f, uint32_t c) {
  if (f & F_WINGS) {
    if (body == B_ANGEL || body == B_KNIGHT) {
      poly(a, WING_ANGEL_L, NPTS(WING_ANGEL_L), c, 3);
      poly(a, WING_ANGEL_R, NPTS(WING_ANGEL_R), c, 3);
    } else if (body == B_DEMON) {
      poly(a, WING_BAT_L, NPTS(WING_BAT_L), c, 3);
      poly(a, WING_BAT_R, NPTS(WING_BAT_R), c, 3);
    } else if (body == B_MECH) {
      poly(a, WING_MECH_L, NPTS(WING_MECH_L), c, 3);
      poly(a, WING_MECH_R, NPTS(WING_MECH_R), c, 3);
    } else if (body == B_BIRD || body == B_INSECT) {
      poly(a, WING_BIRD_L, NPTS(WING_BIRD_L), c, 3);
      poly(a, WING_BIRD_R, NPTS(WING_BIRD_R), c, 3);
    } else {
      poly(a, WING_SMALL_L, NPTS(WING_SMALL_L), c, 3);
      poly(a, WING_SMALL_R, NPTS(WING_SMALL_R), c, 3);
    }
  }
  if (f & F_TAIL) {
    static const int16_t TAIL_BLOB[] = { 28,52, 14,60, 18,74, 30,66 };
    static const int16_t TAIL_DINO[] = { 30,70, 10,76, 16,60 };
    static const int16_t TAIL_QUAD[] = { 22,54, 8,46, 12,60 };
    static const int16_t TAIL_DEFAULT[] = { 28,60, 12,66, 18,78 };
    if (body == B_BLOB) poly(a, TAIL_BLOB, NPTS(TAIL_BLOB), c, 3);
    else if (body == B_DINO) poly(a, TAIL_DINO, NPTS(TAIL_DINO), c, 3);
    else if (body == B_QUAD) poly(a, TAIL_QUAD, NPTS(TAIL_QUAD), c, 3);
    else poly(a, TAIL_DEFAULT, NPTS(TAIL_DEFAULT), c, 3);
  }
  if (f & F_CAPE) {
    static const int16_t CAPE[] = { 34,42, 16,66, 26,86, 40,70 };
    poly(a, CAPE, NPTS(CAPE), c, 3);
  }
  if (f & F_FIN) {
    static const int16_t FIN[] = { 60,44, 76,34, 72,54 };
    poly(a, FIN, NPTS(FIN), c, 3);
  }
}

void drawFront(PetAvatar &a, uint16_t f, int fx, int fy, int spread,
               uint32_t c, uint32_t d) {
  if (f & F_HORN) {
    line(a, coord(a, fx - spread / 2), coord(a, fy - 10),
         coord(a, fx - spread / 2 - 6), coord(a, fy - 22), c, 3);
    line(a, coord(a, fx + spread / 2), coord(a, fy - 10),
         coord(a, fx + spread / 2 + 6), coord(a, fy - 22), c, 3);
  }
  if (f & F_EARS) {
    line(a, coord(a, fx - spread / 2), coord(a, fy - 6),
         coord(a, fx - spread / 2 - 8), coord(a, fy - 22), c, 3);
    line(a, coord(a, fx - spread / 2 - 8), coord(a, fy - 22),
         coord(a, fx - spread / 2 + 2), coord(a, fy - 12), c, 3);
    line(a, coord(a, fx + spread / 2), coord(a, fy - 6),
         coord(a, fx + spread / 2 + 8), coord(a, fy - 22), c, 3);
    line(a, coord(a, fx + spread / 2 + 8), coord(a, fy - 22),
         coord(a, fx + spread / 2 - 2), coord(a, fy - 12), c, 3);
  }
  if (f & F_HALO) {
    poly(a, HALO, NPTS(HALO), d, 2);
  }
  if (f & F_CREST) {
    line(a, coord(a, fx - 4), coord(a, fy - 10),
         coord(a, fx - 10), coord(a, fy - 22), c, 3);
    line(a, coord(a, fx - 10), coord(a, fy - 22),
         coord(a, fx), coord(a, fy - 16), c, 3);
    line(a, coord(a, fx), coord(a, fy - 16),
         coord(a, fx + 8), coord(a, fy - 24), c, 3);
  }
  if (f & F_SPIKES) {
    line(a, coord(a, 66), coord(a, 50), coord(a, 76), coord(a, 43), d, 3);
    line(a, coord(a, 70), coord(a, 60), coord(a, 80), coord(a, 54), d, 3);
    line(a, coord(a, 74), coord(a, 70), coord(a, 84), coord(a, 64), d, 3);
  }
  if (f & F_CANNON) {
    line(a, coord(a, 68), coord(a, 52), coord(a, 84), coord(a, 48), c, 4);
    box(a, 83, 47, 8, 8, d);
  }
  if (f & F_CROWN) {
    static const int16_t CROWN[] = {
      38,20, 42,12, 46,20, 50,12, 54,20, 58,12, 62,20,
    };
    poly(a, CROWN, NPTS(CROWN), d, 3);
  }
  if (f & F_SWORD) {
    line(a, coord(a, 76), coord(a, 60), coord(a, 92), coord(a, 24), d, 4);
    line(a, coord(a, 84), coord(a, 30), coord(a, 94), coord(a, 36), d, 3);
  }
  if (f & F_ANTENNAE) {
    line(a, coord(a, fx - 8), coord(a, fy - 10),
         coord(a, fx - 14), coord(a, fy - 24), c, 3);
    line(a, coord(a, fx + 8), coord(a, fy - 10),
         coord(a, fx + 14), coord(a, fy - 24), c, 3);
    box(a, fx - 14, fy - 25, 4, 4, d);
    box(a, fx + 14, fy - 25, 4, 4, d);
  }
  if (f & F_LEAF) {
    static const int16_t LEAF[] = { 40,36, 22,24, 28,40, 40,44 };
    poly(a, LEAF, NPTS(LEAF), d, 3);
  }
  if (f & F_ARMOR) {
    static const int16_t ARMOR_L[] = { 32,42, 20,38, 22,52, 34,52 };
    static const int16_t ARMOR_R[] = { 68,42, 80,38, 78,52, 66,52 };
    poly(a, ARMOR_L, NPTS(ARMOR_L), d, 3);
    poly(a, ARMOR_R, NPTS(ARMOR_R), d, 3);
  }
  if (f & F_CLAWS) {
    line(a, coord(a, 40), coord(a, 90), coord(a, 36), coord(a, 98), c, 2);
    line(a, coord(a, 44), coord(a, 90), coord(a, 42), coord(a, 98), c, 2);
    line(a, coord(a, 48), coord(a, 90), coord(a, 48), coord(a, 98), c, 2);
  }
}

void redraw(PetAvatar &a) {
  if (!a.buf || !a.image) return;
  memset(a.buf, 0, (size_t)a.size * a.size * 4);

  const int n = (int)(sizeof(SHAPES) / sizeof(SHAPES[0]));
  const ShapeRow &sh = SHAPES[a.form < n ? a.form : 0];
  a.scalePct = sh.scale;

  uint32_t c = argb(a.accent);
  uint32_t d = argb(C_DETAIL);

  drawBack(a, sh.body, sh.feat, c);
  drawBody(a, sh.body, c, d);

  int fx, fy, spread;
  faceFor(sh.body, fx, fy, spread);
  drawFront(a, sh.feat, fx, fy, spread, c, d);
  drawFace(a, fx, fy, spread, a.expr, d);

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

void petAvatarSetSpecies(PetAvatar &a, uint32_t accent, uint8_t form) {
  if (!a.root) return;
  if (a.form == form && a.accent == accent) return;
  a.form = form;
  a.accent = accent;
  lv_obj_set_style_border_color(a.root, lv_color_hex(accent), 0);
  lv_obj_set_style_bg_color(a.root, lv_color_hex(accent), 0);
  if (!a.realImage) redraw(a);
}

void petAvatarSetMood(PetAvatar &a, int mood, bool excited, bool sleeping) {
  if (!a.image) return;
  int e = 0;
  if (sleeping) e = 2;
  else if (excited) e = 1;
  else if (mood <= 1) e = 3;
  if (e != a.expr) {
    a.expr = (uint8_t)e;
    if (!a.realImage) redraw(a);
  }

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
