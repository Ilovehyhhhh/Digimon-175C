#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa el QMI8658 (acelerometro). Devuelve false si no esta presente.
bool imuInit(void);

// Lee la aceleracion en g (aprox -4..4). Devuelve false si no esta listo.
bool imuRead(float *ax, float *ay, float *az);

// Detector de "agitado": devuelve true UNA vez por cada sacudida.
// Llamar periodicamente (p.ej. cada 40-60 ms).
bool imuShake(void);

// Encender/apagar el acelerometro (ahorra energia con la pantalla bloqueada).
void imuSetEnabled(bool on);

#ifdef __cplusplus
}
#endif
