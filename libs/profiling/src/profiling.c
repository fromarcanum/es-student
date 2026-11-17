#include "systime/systime.h"
#include "profiling/profiling.h"

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    const char *name;
    uint64_t start_us;
    uint32_t sum_us;
    uint32_t count;
    uint32_t max_us;
    bool running;
} stopwatch_t;

static stopwatch_t sw[PROFILING_STOPWATCH_COUNT];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_STOPWATCH_COUNT)
        return;

    sw[id] = (stopwatch_t){0};
    sw[id].name = name;
}

#if PROFILING_ENABLED
void profiling_start(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT ||
        sw[id].name == NULL || sw[id].running)
        return;

    sw[id].start_us = systime_us();
    sw[id].running = true;
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_STOPWATCH_COUNT || !sw[id].running)
        return;

    uint64_t elapsed = systime_us() - sw[id].start_us;
    sw[id].running = false;

    uint32_t duration = elapsed > UINT32_MAX
                        ? UINT32_MAX : (uint32_t)elapsed;

    sw[id].count++;
    sw[id].sum_us += duration;

    if (duration > sw[id].max_us)
        sw[id].max_us = duration;
}

#endif

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_STOPWATCH_COUNT ||
        result == NULL || sw[id].name == NULL)
        return false;

    result->name = sw[id].name;
    result->count = sw[id].count;
    result->max_us = sw[id].max_us;
    result->mean_us = sw[id].count
        ? (double)sw[id].sum_us / sw[id].count : 0.0;

    return true;
}

void profiling_reset(void)
{
    uint64_t now = systime_us();

    for (uint32_t i = 0; i < PROFILING_STOPWATCH_COUNT; i++)
    {
        sw[i].count = 0;
        sw[i].sum_us = 0;
        sw[i].max_us = 0;

        if (sw[i].running)
            sw[i].start_us = now;
    }
}
