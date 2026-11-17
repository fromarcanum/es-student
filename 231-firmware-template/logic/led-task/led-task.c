#include "led-task/led-task.h"

#include "platform/platform.h"
#include "systime/systime.h"

#define DEFAULT_PERIOD_MS 1000u

static led_state_t led_state;
static uint32_t led_period_ms;
static uint64_t last_toggle_us;
static bool led_level;
static bool first_blink_toggle;

static void led_output(bool on)
{
    led_level = on;
    platform_led_set(on);
}

void led_task_init(void)
{
    led_period_ms = DEFAULT_PERIOD_MS;
    led_level = false;
    led_output(false);
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    switch (state)
    {
    case LED_STATE_OFF:
        led_state = state;
        led_output(false);
        break;

    case LED_STATE_ON:
        led_state = state;
        led_output(true);
        break;

    case LED_STATE_BLINK:
        led_state = state;
        first_blink_toggle = true;
        last_toggle_us = systime_us();
        break;
    }
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

bool led_task_set_period_ms(uint32_t period_ms)
{
    if (period_ms == 0)
        return false;

    led_period_ms = period_ms;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return led_period_ms;
}

void led_task_handle(void)
{
    if (led_state != LED_STATE_BLINK)
        return;

    uint64_t half_period_us =
        ((uint64_t)led_period_ms * 1000u) / 2u;

    if (first_blink_toggle)
    {
        first_blink_toggle = false;
        led_output(!led_level);
        last_toggle_us = systime_us();
    }
    else if (systime_period_elapsed(
                 &last_toggle_us, half_period_us))
    {
        led_output(!led_level);
    }
}

void led_task_next_state(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;

    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);
        break;

    case LED_STATE_BLINK:
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}
