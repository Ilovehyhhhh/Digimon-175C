#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "bsp/esp-bsp.h"
#include <Arduino.h>

#include "app/digimon.h"
#include "app/digimon_cfg.h"
#include "ui/ui.h"

// Instancia global que espera la capa de compatibilidad (app/compat/Arduino.h).
SerialClass Serial;

static Digimon pet;

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  cfgLoad();
  pet.begin();

  bsp_display_start();

  bsp_display_lock(-1);
  ui_init(pet);
  bsp_display_unlock();

  ESP_LOGI("digimon", "arrancado; LVGL corre el bucle del juego");

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
