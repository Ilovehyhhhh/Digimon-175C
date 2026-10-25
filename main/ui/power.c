#include "power.h"

#include "board.h"

#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// AXP2101 (PMU): registros usados por el firmware de referencia.
//   0x41 INTEN2   bit3 PKEY corto, bit2 PKEY largo
//   0x49 INTSTS2  mismo mapa de bits (escribir 1 para limpiar)
//   0x01 STATUS2  estado de carga / VBUS
//   0x30 ADC_CFG  habilitar medida de VBAT
//   0x34/0x35     voltaje de bateria (13 bits)
//   0xA4          porcentaje (fuel gauge)
//   0x10          COMMON_CFG bit0 = soft shutdown
#define AXP_ADDR        0x34
#define AXP_REG_INTEN2  0x41
#define AXP_REG_INTSTS2 0x49
#define PKEY_SHORT_BIT  0x08
#define PKEY_LONG_BIT   0x04

static const char *TAG = "power";
static i2c_master_dev_handle_t s_axp = nullptr;

static esp_err_t axpRead(uint8_t reg, uint8_t *val) {
  return i2c_master_transmit_receive(s_axp, &reg, 1, val, 1, 100);
}

static esp_err_t axpWrite(uint8_t reg, uint8_t val) {
  uint8_t buf[2] = { reg, val };
  return i2c_master_transmit(s_axp, buf, 2, 100);
}

void powerInit(void) {
  gpio_config_t io = {};
  io.pin_bit_mask = 1ULL << BOARD_PIN_KEY1;
  io.mode = GPIO_MODE_INPUT;
  io.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io);

  i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
  if (!bus) { ESP_LOGE(TAG, "sin bus I2C del BSP"); return; }

  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = AXP_ADDR;
  cfg.scl_speed_hz = 400000;
  if (i2c_master_bus_add_device(bus, &cfg, &s_axp) != ESP_OK) {
    ESP_LOGW(TAG, "AXP2101 @0x34 no accesible");
    s_axp = nullptr;
    return;
  }

  uint8_t en = 0;
  if (axpRead(AXP_REG_INTEN2, &en) == ESP_OK) {
    en |= (PKEY_SHORT_BIT | PKEY_LONG_BIT);
    axpWrite(AXP_REG_INTEN2, en);
    axpWrite(AXP_REG_INTSTS2, PKEY_SHORT_BIT | PKEY_LONG_BIT);
    ESP_LOGI(TAG, "AXP2101 listo (PEK corto/largo)");
  } else {
    ESP_LOGW(TAG, "AXP2101 no responde por I2C");
  }
}

PowerBatt powerBattery(void) {
  PowerBatt b = { 0, -1, false, false };
  if (!s_axp) return b;

  uint8_t v = 0;
  if (axpRead(0x30, &v) == ESP_OK && !(v & 0x01)) axpWrite(0x30, v | 0x01);

  uint8_t hi = 0, lo = 0;
  if (axpRead(0x34, &hi) == ESP_OK && axpRead(0x35, &lo) == ESP_OK) {
    b.mV = ((hi & 0x1F) << 8) | lo;
  }

  uint8_t st = 0;
  if (axpRead(0x01, &st) == ESP_OK) {
    b.charging = (((st >> 5) & 0x03) == 1);
    b.usb = ((st >> 3) & 0x01) == 0 && b.mV > 0;
  }

  uint8_t p = 0;
  if (axpRead(0xA4, &p) == ESP_OK && p <= 100) b.pct = p;
  if (b.pct <= 0 && b.mV > 0) {
    int q = (b.mV - 3200) / 10;
    b.pct = q < 0 ? 0 : (q > 100 ? 100 : q);
  }
  return b;
}

uint8_t powerPoll(void) {
  uint8_t evt = 0;

  static bool bootPrev = false;
  bool boot = (gpio_get_level((gpio_num_t)BOARD_PIN_KEY1) == 0);
  if (!boot && bootPrev) evt |= PEV_A;
  bootPrev = boot;

  uint8_t st = 0;
  if (s_axp && axpRead(AXP_REG_INTSTS2, &st) == ESP_OK && st) {
    if (st & PKEY_SHORT_BIT) { evt |= PEV_B; axpWrite(AXP_REG_INTSTS2, PKEY_SHORT_BIT); }
    if (st & PKEY_LONG_BIT)  { evt |= PEV_SCREEN; axpWrite(AXP_REG_INTSTS2, PKEY_LONG_BIT); }
  }
  return evt;
}

void powerOff(void) {
  if (!s_axp) { ESP_LOGW(TAG, "sin AXP, no puedo apagar"); return; }
  uint8_t v = 0;
  if (axpRead(0x10, &v) == ESP_OK) {
    axpWrite(0x10, (uint8_t)(v | 0x01));
    ESP_LOGI(TAG, "apagando (AXP soft shutdown)");
  }
}
