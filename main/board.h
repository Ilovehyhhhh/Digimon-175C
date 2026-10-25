#pragma once

// Perfil de hardware: Waveshare ESP32-S3-Touch-AMOLED-1.75C.
//
// Datos tomados del board header del proyecto de referencia
// claude-desktop-buddy-esp32:
//   src/boards/board_waveshare_esp32s3_touch_amoled_1_75c.h
//
// El display (CO5300 QSPI), el touch (CST9217) y el bus I2C compartido los
// inicializa el BSP oficial de Waveshare. Este archivo solo documenta el
// cableado y expone los pocos pines que usamos directamente (BOOT / IMU).
// No cambiar los pines a mano sin revisar el esquematico de la placa.

#define BOARD_NAME "Waveshare ESP32-S3-Touch-AMOLED-1.75C"

// Panel fisico: AMOLED redondo 466x466 (CO5300 por QSPI).
#define BOARD_LCD_W 466
#define BOARD_LCD_H 466

// QSPI del panel.
#define BOARD_PIN_LCD_SDIO0 4
#define BOARD_PIN_LCD_SDIO1 5
#define BOARD_PIN_LCD_SDIO2 6
#define BOARD_PIN_LCD_SDIO3 7
#define BOARD_PIN_LCD_SCLK  38
#define BOARD_PIN_LCD_CS    12
#define BOARD_PIN_LCD_RST   1

// Bus I2C compartido: AXP2101, CST9217, QMI8658, ES8311.
#define BOARD_PIN_I2C_SDA 15
#define BOARD_PIN_I2C_SCL 14

// Touch.
#define BOARD_PIN_TP_RST 2
#define BOARD_PIN_TP_INT 11

// Audio (ES8311 + amplificador).
#define BOARD_PIN_I2S_MCLK 16
#define BOARD_PIN_I2S_BCLK 9
#define BOARD_PIN_I2S_WS   45
#define BOARD_PIN_I2S_DI   10
#define BOARD_PIN_I2S_DO   8
#define BOARD_PIN_PA_CTRL  46

// Boton BOOT (activo a nivel bajo). La tecla PWR cuelga del AXP2101.
#define BOARD_PIN_KEY1 0

#define BOARD_HAS_PSRAM      1
#define BOARD_HAS_AXP2101    1
#define BOARD_HAS_IMU        1
#define BOARD_HAS_ES8311     1
#define BOARD_HAS_RTC        0
#define BOARD_HAS_SD_CARD    0
#define BOARD_HAS_TCA9554    0
