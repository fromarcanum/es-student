#include "firmware.h"
#include "platform/platform.h"

#include "logging/log.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "edge-task/edge-task.h"
#include "uart-task/uart-task.h"
#include "block-task/block-task.h"

typedef enum
{
    SW_LOOP,
    SW_COMMAND,
    SW_LED,
    SW_BUTTON,
    SW_PI,
    SW_EDGE,
    SW_UART,
    SW_BLOCK
} stopwatch_id_t;

int main(void)
{
    platform_init();

    LOG_INF("%s %s\n",
            FIRMWARE_NAME,
            FIRMWARE_VERSION);

    stdio_text_protocol_init();
    led_task_init();
    pi_task_init();

    button_task_init(led_task_next_state);
    profiling_stopwatch_init(SW_LOOP, "loop");
    profiling_stopwatch_init(SW_COMMAND, "command");
    profiling_stopwatch_init(SW_LED, "led");
    profiling_stopwatch_init(SW_BUTTON, "button");
    profiling_stopwatch_init(SW_PI, "pi");
    profiling_stopwatch_init(SW_EDGE, "edge");
    profiling_stopwatch_init(SW_UART, "uart");
    profiling_stopwatch_init(SW_BLOCK, "block");

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

        profiling_start(SW_EDGE);
        edge_task_handle();
        profiling_stop(SW_EDGE);

        profiling_start(SW_UART);
        uart_task_handle();
        profiling_stop(SW_UART);

        profiling_start(SW_BLOCK);
        block_task_handle();
        profiling_stop(SW_BLOCK);

        profiling_start(SW_COMMAND);
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
            api_handle(command);
        profiling_stop(SW_COMMAND);

        profiling_stop(SW_LOOP);
    }
}
