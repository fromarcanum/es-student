#pragma once

#include <stdbool.h>
#include <stdint.h>

// Сумма замеров хранится в uint32_t. Переполнение возможно
// примерно через 71 минуту накопленного времени замеров.

#define PROFILING_STOPWATCH_COUNT 5u

typedef struct
{
    const char *name;
    uint32_t count;
    double mean_us;
    uint32_t max_us;
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);
