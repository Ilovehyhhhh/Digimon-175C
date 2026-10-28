#include "audio.h"

#include "bsp/esp-bsp.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define AUDIO_SR 22050
#define AUDIO_MAX 2048
#define AUDIO_PI 3.14159265f

static const char *TAG = "audio";
static esp_codec_dev_handle_t s_spk = NULL;
static TaskHandle_t s_task = NULL;
static volatile uint8_t s_req = 0;
static int16_t s_buf[AUDIO_MAX];
static esp_codec_dev_sample_info_t s_fs;

static int make_tone(uint8_t req) {
  int n;
  float amp;
  float dur;
  if (req == 0) {
    n = AUDIO_SR * 22 / 1000;
    amp = 2600.0f;
    dur = 0.022f;
  } else {
    n = AUDIO_SR * 90 / 1000;
    amp = 3400.0f;
    dur = 0.090f;
  }
  if (n > AUDIO_MAX) n = AUDIO_MAX;

  float phase = 0.0f;
  for (int i = 0; i < n; i++) {
    float t = (float)i / AUDIO_SR;
    float f;
    if (req == 0) {
      f = 2200.0f;
    } else {
      f = 900.0f + 750.0f * sinf(2.0f * AUDIO_PI * 2.4f * t);
    }
    phase += 2.0f * AUDIO_PI * f / AUDIO_SR;
    float env = 1.0f - (t / dur);
    if (env < 0.0f) env = 0.0f;
    s_buf[i] = (int16_t)(sinf(phase) * amp * env);
  }
  return n;
}

static bool ensure_speaker(void) {
  if (s_spk) return true;
  s_spk = bsp_audio_codec_speaker_init();
  if (!s_spk) {
    ESP_LOGW(TAG, "sin codec de audio");
    return false;
  }
  memset(&s_fs, 0, sizeof(s_fs));
  s_fs.sample_rate = AUDIO_SR;
  s_fs.channel = 1;
  s_fs.bits_per_sample = 16;
  return true;
}

static void audioTask(void *) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (!ensure_speaker()) continue;
    if (esp_codec_dev_open(s_spk, &s_fs) != 0) {
      ESP_LOGW(TAG, "no se pudo abrir el codec");
      continue;
    }
    esp_codec_dev_set_out_vol(s_spk, 65);
    int n = make_tone(s_req);
    esp_codec_dev_write(s_spk, s_buf, n * 2);
    vTaskDelay(pdMS_TO_TICKS(20));
    esp_codec_dev_close(s_spk);
  }
}

void audioInit(void) {
  if (xTaskCreate(audioTask, "audio", 4096, NULL, 4, &s_task) != pdPASS) {
    ESP_LOGW(TAG, "sin tarea de audio");
    s_task = NULL;
  }
}

void audioChirp(void) {
  if (!s_task) return;
  s_req = 1;
  xTaskNotifyGive(s_task);
}

void audioClick(void) {
  if (!s_task) return;
  s_req = 0;
  xTaskNotifyGive(s_task);
}
