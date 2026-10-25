# Boilerplate de ESP-IDF (orden exacto obligatorio para cmake)
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-Wno-format)
project(digimon_pet)
