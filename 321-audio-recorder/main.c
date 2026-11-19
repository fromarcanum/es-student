#include "firmware.h"
#include "platform/platform.h"

#include "logging/log.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"

#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "telemetry-task/telemetry-task.h"
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"

typedef enum
{
    SW_LOOP,
    SW_COMMAND,
    SW_LED,
    SW_BUTTON,
    SW_PI,
    STOPWATCH_TELEMETRY,
    STOPWATCH_BLOCK,
    STOPWATCH_STREAM
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
    telemetry_task_init(api_print_telemetry);
    stream_task_init(api_send_frame);

    // Связываем кнопку со светодиодом.
    button_task_init(led_task_next_state);

    profiling_stopwatch_init(SW_LOOP, "loop");
    profiling_stopwatch_init(SW_COMMAND, "command");
    profiling_stopwatch_init(SW_LED, "led");
    profiling_stopwatch_init(SW_BUTTON, "button");
    profiling_stopwatch_init(SW_PI, "pi");
    profiling_stopwatch_init(STOPWATCH_TELEMETRY, "telemetry");
    profiling_stopwatch_init(STOPWATCH_BLOCK, "block");
    profiling_stopwatch_init(STOPWATCH_STREAM, "stream");

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

        profiling_start(STOPWATCH_TELEMETRY);
        telemetry_task_handle();
        profiling_stop(STOPWATCH_TELEMETRY);

        profiling_start(STOPWATCH_BLOCK);
        block_task_handle();
        profiling_stop(STOPWATCH_BLOCK);

        profiling_start(STOPWATCH_STREAM);
        stream_task_handle();
        profiling_stop(STOPWATCH_STREAM);

        profiling_start(SW_COMMAND);

        const command_t *command =
            stdio_text_protocol_handle();

        if (command != NULL)
            api_handle(command);

        profiling_stop(SW_COMMAND);
        profiling_stop(SW_LOOP);
    }
}
