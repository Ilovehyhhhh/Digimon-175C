#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Botones y energia de la 1.75C (datos de claude-desktop-buddy / board.h):
//   BOOT (GPIO0)       -> accion "A" (corta): abrir menu / volver
//   PWR  (AXP PEK)     -> accion "B" (corta): bloquear / desbloquear
//                         pulsacion larga: apagar (PEV_SCREEN)
//                         pulsacion muy larga: apagado por hardware (AXP)
//
// La UI decide que hacer con cada evento; este modulo solo los detecta.

enum {
  PEV_A = 1 << 0,       // BOOT pulsacion corta
  PEV_B = 1 << 1,       // PWR pulsacion corta (AXP PEK)
  PEV_SCREEN = 1 << 2,  // PWR pulsacion larga -> apagar
};

void powerInit(void);
uint8_t powerPoll(void);
void powerOff(void);

typedef struct {
  int  mV;
  int  pct;
  bool charging;
  bool usb;
} PowerBatt;

PowerBatt powerBattery(void);

#ifdef __cplusplus
}
#endif
