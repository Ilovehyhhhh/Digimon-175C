#pragma once
// Capa de compatibilidad Arduino -> ESP-IDF, SOLO para la capa de negocio.
// La capa de negocio (app/) usa millis(), random(), min/max, delay() y Serial;
// aqui se mapean a los equivalentes de ESP-IDF. NO se usa para UI/drivers.

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdarg.h>

#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ---- tipos / utilidades ----
using String = std::string;

// millis(): igual que Arduino, entero de 32 bits que da la vuelta a ~49 dias.
inline uint32_t millis() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

inline void delay(uint32_t ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

inline long random(long max) {
  return (max <= 0) ? 0 : (long)(esp_random() % (uint32_t)max);
}

inline long random(long min, long max) {
  return (max <= min) ? min : (min + random(max - min));
}

using std::min;
using std::max;

// ---- macros de la API de Arduino que aparecen en el codigo ----
#ifndef snprintf_P
#define snprintf_P snprintf
#endif
#ifndef PSTR
#define PSTR(s) (s)
#endif
#ifndef F
#define F(s) (s)
#endif
#ifndef constrain
#define constrain(x, a, b) (((x) < (a)) ? (a) : ((x) > (b) ? (b) : (x)))
#endif

// ---- Serial (solo para logs de la capa de negocio) ----
struct SerialClass {
  void begin(unsigned long) {}
  void end() {}
  void setRxBufferSize(int) {}
  void setTxTimeoutMs(uint32_t) {}
  void println(const char *s = "") { ESP_LOGI("pet", "%s", s); }
  void printf(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    va_list args;
    va_start(args, fmt);
    char buf[160];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ESP_LOGI("pet", "%s", buf);
  }
};
extern SerialClass Serial;
