#pragma once
// Preferences (API de Arduino) respaldado por NVS de ESP-IDF.
// Solo se usa en la capa de negocio; la UI/drivers usan NVS/esp_lcd directo.

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

class Preferences {
 public:
  bool begin(const char *ns, bool readOnly = false) {
    end();
    esp_err_t err = nvs_open(ns, readOnly ? NVS_READONLY : NVS_READWRITE, &handle_);
    if (err != ESP_OK) {
      handle_ = 0;
      return false;
    }
    return true;
  }

  void end() {
    if (handle_) {
      nvs_close(handle_);
      handle_ = 0;
    }
  }

  void clear() {
    if (handle_) nvs_erase_all(handle_);
  }

  // Fuerza la escritura a flash (nvs_commit). Llamar una vez por guardado.
  void commit() {
    if (handle_) nvs_commit(handle_);
  }

  bool isKey(const char *key) const {
    if (!handle_) return false;
    nvs_type_t type = NVS_TYPE_ANY;
    return nvs_find_key(handle_, key, &type) == ESP_OK;
  }

  void putBool(const char *key, bool value) {
    if (handle_) nvs_set_u8(handle_, key, value ? 1 : 0);
  }
  bool getBool(const char *key, bool fallback = false) const {
    uint8_t v = 0;
    return (handle_ && nvs_get_u8(handle_, key, &v) == ESP_OK) ? (v != 0) : fallback;
  }

  void putUChar(const char *key, uint8_t value) {
    if (handle_) nvs_set_u8(handle_, key, value);
  }
  uint8_t getUChar(const char *key, uint8_t fallback = 0) const {
    uint8_t v = 0;
    return (handle_ && nvs_get_u8(handle_, key, &v) == ESP_OK) ? v : fallback;
  }

  void putUInt(const char *key, uint32_t value) {
    if (handle_) nvs_set_u32(handle_, key, value);
  }
  uint32_t getUInt(const char *key, uint32_t fallback = 0) const {
    uint32_t v = 0;
    return (handle_ && nvs_get_u32(handle_, key, &v) == ESP_OK) ? v : fallback;
  }

  void putShort(const char *key, int16_t value) {
    if (handle_) nvs_set_i16(handle_, key, value);
  }
  int16_t getShort(const char *key, int16_t fallback = 0) const {
    int16_t v = 0;
    return (handle_ && nvs_get_i16(handle_, key, &v) == ESP_OK) ? v : fallback;
  }

  void putUShort(const char *key, uint16_t value) {
    if (handle_) nvs_set_u16(handle_, key, value);
  }
  uint16_t getUShort(const char *key, uint16_t fallback = 0) const {
    uint16_t v = 0;
    return (handle_ && nvs_get_u16(handle_, key, &v) == ESP_OK) ? v : fallback;
  }

  int8_t getChar(const char *key, int8_t fallback = 0) const {
    int8_t v = 0;
    return (handle_ && nvs_get_i8(handle_, key, &v) == ESP_OK) ? v : fallback;
  }

  void putString(const char *key, const char *value) {
    if (handle_ && value) nvs_set_str(handle_, key, value);
  }
  void getString(const char *key, char *out, size_t len) const {
    if (len == 0) return;
    out[0] = '\0';
    size_t needed = len;
    if (handle_ && nvs_get_str(handle_, key, nullptr, &needed) == ESP_OK && needed > 0) {
      char *tmp = (char *)malloc(needed);
      if (tmp) {
        if (nvs_get_str(handle_, key, tmp, &needed) == ESP_OK) {
          strncpy(out, tmp, len - 1);
          out[len - 1] = '\0';
        }
        free(tmp);
      }
    }
  }

  void putBytes(const char *key, const void *data, size_t len) {
    if (handle_ && data && len) nvs_set_blob(handle_, key, data, len);
  }
  size_t getBytesLength(const char *key) const {
    if (!handle_) return 0;
    size_t len = 0;
    return nvs_get_blob(handle_, key, nullptr, &len) == ESP_OK ? len : 0;
  }
  size_t getBytes(const char *key, void *out, size_t len) const {
    if (!handle_ || !out || len == 0) {
      if (out && len) memset(out, 0, len);
      return 0;
    }
    size_t required = len;
    esp_err_t err = nvs_get_blob(handle_, key, out, &required);
    if (err == ESP_OK) {
      return required;  // <= len
    }
    if (err == ESP_ERR_NVS_INVALID_LENGTH) {
      // el blob es mas grande que el buffer: lee completo y trunca (como Arduino)
      void *tmp = malloc(required);
      if (!tmp) { memset(out, 0, len); return 0; }
      size_t r2 = required;
      if (nvs_get_blob(handle_, key, tmp, &r2) == ESP_OK) {
        memcpy(out, tmp, len);
      } else {
        memset(out, 0, len);
      }
      free(tmp);
      return len;
    }
    memset(out, 0, len);
    return 0;
  }

 private:
  nvs_handle_t handle_ = 0;
};
