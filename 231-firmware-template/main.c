#include <stdint.h>

#include "pico/stdlib.h"
#include "led.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"

#define BLINK_HALF_PERIOD_US 500000u

static uint64_t last_toggle_us;

static void led_task(void)
{
    uint64_t now = time_us_64();

    if (now - last_toggle_us >= BLINK_HALF_PERIOD_US)
    {
        last_toggle_us = now;
        led_toggle();
    }
}

int main(void)
{
    stdio_init_all();
    led_init();
    profiling_init();
    stdio_text_protocol_init();

    last_toggle_us = time_us_64();

    while (1)
    {
        profiling_iteration();
        led_task();

        const command_t *command =
            stdio_text_protocol_handle();

        if (command != NULL)
        {
            api_handle(command);
        }
    }
}
