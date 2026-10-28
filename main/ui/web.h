#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "app/digimon.h"

// Servidor web local (AP) para exportar/importar el save, editar valores y
// subir firmware por OTA. Se inicia solo desde la pantalla "网页管理".
bool webStart(Digimon *pet);
void webStop(void);
bool webIsActive(void);
uint32_t webLastActivity(void);

const char *webSsid(void);
const char *webPassword(void);
