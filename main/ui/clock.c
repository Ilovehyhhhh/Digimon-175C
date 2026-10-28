#include "clock.h"

#include "esp_log.h"
#include "nvs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "clock";
static nvs_handle_t s_nvs = 0;

static bool valid_epoch(time_t t) {
  return t > 1735689600;  // 2025-01-01
}

static void set_epoch(time_t t) {
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, NULL);
}

static time_t build_time(void) {
  static const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };
  char mon[4] = "";
  int day = 1, year = 2026, hh = 12, mm = 0, ss = 0;
  sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);

  int mi = 0;
  for (int i = 0; i < 12; i++) {
    if (strcmp(mon, months[i]) == 0) { mi = i; break; }
  }

  struct tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = mi;
  t.tm_mday = day;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  t.tm_isdst = -1;
  return mktime(&t);
}

void clockInit(void) {
  setenv("TZ", "CST-8", 1);
  tzset();

  if (nvs_open("clock", NVS_READWRITE, &s_nvs) != ESP_OK) {
    s_nvs = 0;
    ESP_LOGW(TAG, "NVS clock no disponible");
  }

  uint32_t saved = 0;
  if (s_nvs && nvs_get_u32(s_nvs, "epoch", &saved) == ESP_OK &&
      valid_epoch((time_t)saved)) {
    set_epoch((time_t)saved);
  } else {
    time_t t = build_time();
    if (valid_epoch(t)) set_epoch(t);
  }

  char hm[8] = "--:--";
  if (clockGetHM(hm, sizeof(hm))) ESP_LOGI(TAG, "hora inicial %s", hm);
}

bool clockGetHM(char *buf, size_t n) {
  if (!buf || n < 6) return false;
  time_t now = time(NULL);
  if (!valid_epoch(now)) {
    snprintf(buf, n, "--:--");
    return false;
  }
  struct tm t;
  localtime_r(&now, &t);
  snprintf(buf, n, "%02d:%02d", t.tm_hour, t.tm_min);
  return true;
}

bool clockGetParts(int *hour, int *minute) {
  time_t now = time(NULL);
  if (!valid_epoch(now)) {
    if (hour) *hour = 12;
    if (minute) *minute = 0;
    return false;
  }
  struct tm t;
  localtime_r(&now, &t);
  if (hour) *hour = t.tm_hour;
  if (minute) *minute = t.tm_min;
  return true;
}

void clockSave(void) {
  if (!s_nvs) return;
  time_t now = time(NULL);
  if (!valid_epoch(now)) return;
  nvs_set_u32(s_nvs, "epoch", (uint32_t)now);
  nvs_commit(s_nvs);
}

uint32_t clockGetEpoch(void) {
  time_t now = time(NULL);
  return valid_epoch(now) ? (uint32_t)now : 0;
}

void clockSetEpoch(uint32_t epoch) {
  if (!valid_epoch((time_t)epoch)) return;
  set_epoch((time_t)epoch);
  clockSave();
}

void clockAdjust(int dh, int dm) {
  time_t now = time(NULL);
  if (!valid_epoch(now)) now = build_time();
  struct tm t;
  localtime_r(&now, &t);
  t.tm_hour += dh;
  t.tm_min += dm;
  t.tm_isdst = -1;
  time_t next = mktime(&t);
  if (next > 0) set_epoch(next);
  clockSave();
}
