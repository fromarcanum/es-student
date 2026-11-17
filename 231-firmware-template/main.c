#include "pico/stdlib.h"

#include "device.h"
#include "log.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

typedef enum
{
    SW_LOOP,
    SW_COMMAND,
    SW_LED,
    SW_BUTTON,
    SW_PI
} stopwatch_id_t;

int main(void)
{
    stdio_init_all();

    LOG_INF("%s %s\n", DEVICE_NAME, FIRMWARE_VERSION);

    stdio_text_protocol_init();
    led_task_init();
    button_task_init();
    pi_task_init();

    profiling_stopwatch_init(SW_LOOP, "loop");
    profiling_stopwatch_init(SW_COMMAND, "command");
    profiling_stopwatch_init(SW_LED, "led");
    profiling_stopwatch_init(SW_BUTTON, "button");
    profiling_stopwatch_init(SW_PI, "pi");

    while (1)
    {
        profiling_start(SW_LOOP);

        profiling_start(SW_LED);
        led_task_handle();
        profiling_stop(SW_LED);

        profiling_start(SW_BUTTON);
        button_task_handle();
        profiling_stop(SW_BUTTON);

        profiling_start(SW_PI);
        pi_task_handle();
        profiling_stop(SW_PI);

        profiling_start(SW_COMMAND);

        const command_t *command =
            stdio_text_protocol_handle();

        if (command != NULL)
            api_handle(command);

        profiling_stop(SW_COMMAND);
        profiling_stop(SW_LOOP);
    }
}
