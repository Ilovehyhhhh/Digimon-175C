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
#define AUDIO_MAX 4096
#define AUDIO_PI 3.14159265f

static const char *TAG = "audio";
static esp_codec_dev_handle_t s_spk = NULL;
static TaskHandle_t s_task = NULL;
static volatile uint8_t s_req = 0;
static int16_t s_buf[AUDIO_MAX];

static int make_tone(uint8_t req) {
  int n;
  float amp;
  float dur;
  if (req == 0) {           // click
    n = AUDIO_SR * 45 / 1000;
    amp = 2600.0f;
    dur = 0.045f;
  } else {                  // chirp
    n = AUDIO_SR * 180 / 1000;
    amp = 3400.0f;
    dur = 0.180f;
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

static void audioTask(void *) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (!s_spk) continue;
    int n = make_tone(s_req);
    esp_codec_dev_write(s_spk, s_buf, n * 2);
  }
}

void audioInit(void) {
  s_spk = bsp_audio_codec_speaker_init();
  if (!s_spk) {
    ESP_LOGW(TAG, "sin codec de audio");
    return;
  }

  esp_codec_dev_sample_info_t fs = {0};
  fs.sample_rate = AUDIO_SR;
  fs.channel = 1;
  fs.bits_per_sample = 16;
  if (esp_codec_dev_open(s_spk, &fs) != 0) {
    ESP_LOGW(TAG, "no se pudo abrir el codec");
    s_spk = NULL;
    return;
  }
  esp_codec_dev_set_out_vol(s_spk, 65);

  if (xTaskCreate(audioTask, "audio", 4096, NULL, 4, &s_task) != pdPASS) {
    ESP_LOGW(TAG, "sin tarea de audio");
    s_task = NULL;
  }
  ESP_LOGI(TAG, "audio listo");
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


