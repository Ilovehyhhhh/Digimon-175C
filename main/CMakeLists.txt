# Mascota virtual (ESP-IDF) para Waveshare ESP32-S3-Touch-AMOLED-1.75C.
#
# app/  = logica pura de la mascota (sin pantalla ni drivers).
# ui/   = LVGL + drivers de la placa (power/imu/fuentes).
# El display CO5300 y el touch CST9217 los inicializa el BSP de Waveshare.
idf_component_register(
    SRCS
        "main.cpp"
        "app/digimon.cpp"
        "app/digimon_cfg.cpp"
        "app/battle.cpp"
        "ui/ui.cpp"
        "ui/pet_avatar.cpp"
        "ui/clock.c"
        "ui/audio.c"
        "ui/web.cpp"
        "ui/power.c"
        "ui/imu.c"
        "ui/digifont.c"
        "ui/digifont_small.c"
        "ui/digifont_big.c"
    INCLUDE_DIRS
        "."
        "ui"
        "app"
        "app/compat"
    REQUIRES
        waveshare__esp32_s3_touch_amoled_1_75c
        lvgl
        nvs_flash
        esp_timer
        cjson
        esp_wifi
        esp_netif
        esp_event
        esp_http_server
        app_update
)

# LVGL con CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM=y emite secciones IRAM que
# chocan entre si en algunos archivos; el BSP oficial de Waveshare las silencia
# igual. Sin esto la compilacion falla con -Werror=attributes.
idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
if(TARGET ${LVGL_LIB})
    target_compile_options(${LVGL_LIB} PRIVATE -Wno-error=attributes -Wno-error=cpp)
endif()
