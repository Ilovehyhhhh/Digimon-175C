#include "imu.h"

#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>

// QMI8658 (acelerometro 6 ejes) en el bus I2C del BSP (SDA=15, SCL=14).
#define QMI_ADDR    0x6B
#define QMI_WHOAMI  0x00
#define QMI_CTRL1   0x02
#define QMI_CTRL2   0x03
#define QMI_CTRL7   0x08
#define QMI_AX_L    0x35

static const char *TAG = "imu";
static i2c_master_dev_handle_t s_imu = nullptr;
static bool s_ok = false;

static esp_err_t regRead(uint8_t reg, uint8_t *val, size_t n) {
  return i2c_master_transmit_receive(s_imu, &reg, 1, val, n, 100);
}
static esp_err_t regWrite(uint8_t reg, uint8_t val) {
  uint8_t buf[2] = { reg, val };
  return i2c_master_transmit(s_imu, buf, 2, 100);
}

bool imuInit(void) {
  i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
  if (!bus) { ESP_LOGE(TAG, "sin bus I2C del BSP"); return false; }

  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = QMI_ADDR;
  cfg.scl_speed_hz = 400000;
  if (i2c_master_bus_add_device(bus, &cfg, &s_imu) != ESP_OK) {
    ESP_LOGW(TAG, "QMI8658 @0x6B no accesible");
    s_imu = nullptr;
    return false;
  }

  uint8_t who = 0;
  if (regRead(QMI_WHOAMI, &who, 1) != ESP_OK || who != 0x05) {
    ESP_LOGW(TAG, "QMI8658 WHO_AM_I=0x%02X (esperado 0x05)", who);
    return false;
  }

  regWrite(QMI_CTRL7, 0x00);  // deshabilitar
  regWrite(QMI_CTRL1, 0x40);  // ADDR_AI (auto-incremento para lectura en bloque)
  regWrite(QMI_CTRL2, 0x16);  // +-4g, 125 Hz  => (1<<4)|0x06
  regWrite(QMI_CTRL7, 0x01);  // habilitar acelerometro
  vTaskDelay(pdMS_TO_TICKS(10));

  s_ok = true;
  ESP_LOGI(TAG, "QMI8658 listo (WHO=0x%02X, 4g/125Hz)", who);
  return true;
}

bool imuRead(float *ax, float *ay, float *az) {
  if (!s_ok) return false;
  uint8_t d[6];
  if (regRead(QMI_AX_L, d, 6) != ESP_OK) return false;
  int16_t x = (int16_t)((uint16_t)d[0] | ((uint16_t)d[1] << 8));
  int16_t y = (int16_t)((uint16_t)d[2] | ((uint16_t)d[3] << 8));
  int16_t z = (int16_t)((uint16_t)d[4] | ((uint16_t)d[5] << 8));
  const float sc = 4.0f / 32768.0f;
  if (ax) *ax = x * sc;
  if (ay) *ay = y * sc;
  if (az) *az = z * sc;
  return true;
}

bool imuShake(void) {
  static float lp = 1.0f;
  static bool active = false;
  static int64_t lastShake = 0;

  float ax, ay, az;
  if (!imuRead(&ax, &ay, &az)) return false;

  float mag = sqrtf(ax * ax + ay * ay + az * az);
  lp += (mag - lp) * 0.2f;               // estimacion suave de la gravedad
  float jolt = fabsf(mag - lp);          // componente dinamica

  int64_t now = esp_timer_get_time();
  if (jolt > 0.9f && !active && (now - lastShake) > 700000) {
    active = true;
    lastShake = now;
    return true;
  }
  if (jolt < 0.35f) active = false;
  return false;
}
