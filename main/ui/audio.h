#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Sonidos cortos por ES8311/I2S. Se reproducen en una tarea aparte para no
// bloquear LVGL.
void audioInit(void);
void audioChirp(void);   // "llamada" de la mascota
void audioClick(void);   // pulsacion de boton

#ifdef __cplusplus
}
#endif
