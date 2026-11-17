#include "pi-task.h"

#include "pico/stdlib.h"

// При 125 МГц один член ряда занимает около 4,7 мкс.
// 64 * 4,7 = 300,8 мкс < 500 мкс.
#define PI_TERMS_PER_CALL 64u

static pi_state_t pi_state;

static uint32_t pi_terms;
static uint32_t pi_k;

static double pi_sum;
static double pi_sign;
static double pi_result;

static uint64_t pi_start_us;
static uint64_t pi_elapsed_ms;

void pi_task_init(void)
{
    pi_state = PI_NOT_STARTED;

    pi_terms = 0;
    pi_k = 0;

    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;

    pi_start_us = 0;
    pi_elapsed_ms = 0;
}

bool pi_task_start(uint32_t terms)
{
    if (pi_state == PI_RUNNING || terms == 0)
        return false;

    pi_terms = terms;
    pi_k = 0;

    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    pi_elapsed_ms = 0;

    pi_start_us = time_us_64();
    pi_state = PI_RUNNING;

    return true;
}

void pi_task_handle(void)
{
    if (pi_state != PI_RUNNING)
        return;

    // Вычисляем размер порции через остаток.
    // Так не возникает переполнения при UINT32_MAX.
    uint32_t remaining = pi_terms - pi_k;
    uint32_t count = remaining;

    if (count > PI_TERMS_PER_CALL)
        count = PI_TERMS_PER_CALL;

    for (uint32_t i = 0; i < count; i++)
    {
        pi_sum += pi_sign /
                  (2.0 * (double)pi_k + 1.0);

        pi_sign = -pi_sign;
        pi_k++;
    }

    if (pi_k == pi_terms)
    {
        pi_result = 4.0 * pi_sum;

        pi_elapsed_ms =
            (time_us_64() - pi_start_us) / 1000;

        pi_state = PI_READY;
    }
}

pi_state_t pi_task_get_state(void)
{
    return pi_state;
}

bool pi_task_get_result(double *value,
                        uint64_t *elapsed_ms)
{
    if (pi_state != PI_READY)
        return false;

    if (value != NULL)
        *value = pi_result;

    if (elapsed_ms != NULL)
        *elapsed_ms = pi_elapsed_ms;

    return true;
}
