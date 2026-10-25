#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reloj de software (la placa no tiene RTC externo).
// Se inicializa con la hora de compilacion la primera vez y se puede ajustar
// desde la pantalla de sistema. La ultima hora se guarda en NVS.
void clockInit(void);
bool clockGetHM(char *buf, size_t n);
bool clockGetParts(int *hour, int *minute);
void clockAdjust(int dh, int dm);
void clockSave(void);

#ifdef __cplusplus
}
#endif
