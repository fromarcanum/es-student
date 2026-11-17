#pragma once

#include <stdint.h>

#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.1.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/fromarcanum/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

typedef struct
{
    const char *name;
    const char *version;
    const char *project;
    const char *repo;
    const char *board;
    const char *sdk_version;

    char serial[17];

    uint32_t manufacturer;
    uint32_t part;
    uint32_t revision;
} device_info_t;

void device_get_info(device_info_t *info);
