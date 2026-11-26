#include "edge-task/edge-task.h"
#include "systime/systime.h"
#include <stddef.h>

// Напряжение GP26 считывает platform. Задача ничего не знает о Pico SDK.
#define EMPTY_V 0.05f
#define RISE_V  2.0f
#define FULL_V  3.2f
#define FALL_V  1.3f
#define TIMEOUT_US 2000000ull

typedef enum {
    EDGE_IDLE,
    EDGE_DISCHARGE,
    EDGE_RISE,
    EDGE_CHARGE,
    EDGE_FALL
} edge_state_t;

static edge_state_t state = EDGE_IDLE;
static platform_line_mode_t current_mode;
static edge_task_callback_t on_finished;
static uint64_t phase_start_us;
static uint32_t measured_rise_us;

static void enter(edge_state_t next, bool high)
{
    platform_line_set(current_mode, high);
    phase_start_us = systime_us();
    state = next;
}

static void finish(bool ok, uint32_t fall_us)
{
    // Вернуть GP22 в ноль даже при таймауте.
    platform_line_set(current_mode, false);
    edge_task_callback_t callback = on_finished;
    platform_line_mode_t mode = current_mode;
    uint32_t rise_us = measured_rise_us;
    state = EDGE_IDLE;
    on_finished = NULL;

    // Задача уже свободна: callback может начать новый замер.
    if (callback != NULL)
        callback(mode, ok, ok ? rise_us : 0u, ok ? fall_us : 0u);
}

bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done)
{
    if (state != EDGE_IDLE || on_done == NULL)
        return false;

    current_mode = mode;
    on_finished = on_done;
    measured_rise_us = 0;
    enter(EDGE_DISCHARGE, false);
    return true;
}

void edge_task_handle(void)
{
    if (state == EDGE_IDLE)
        return;

    uint64_t now = systime_us();
    if (now - phase_start_us > TIMEOUT_US)
    {
        finish(false, 0u);
        return;
    }

    float voltage = platform_voltage_read();
    switch (state)
    {
    case EDGE_DISCHARGE:
        if (voltage <= EMPTY_V)
            enter(EDGE_RISE, true);
        break;
    case EDGE_RISE:
        if (voltage >= RISE_V)
        {
            measured_rise_us = (uint32_t)(now - phase_start_us);
            // Продолжаем заряд до 3,2 В, перед измерением спада.
            phase_start_us = now;
            state = EDGE_CHARGE;
        }
        break;
    case EDGE_CHARGE:
        if (voltage >= FULL_V)
            enter(EDGE_FALL, false);
        break;
    case EDGE_FALL:
        if (voltage <= FALL_V)
            finish(true, (uint32_t)(now - phase_start_us));
        break;
    case EDGE_IDLE:
        break;
    }
}
