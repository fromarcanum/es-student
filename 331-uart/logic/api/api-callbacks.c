#include "api/api-commands.h"

#include "systime/systime.h"
#include "firmware.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "device/device.h"
#include "profiling/profiling.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "platform/platform.h"
#include "edge-task/edge-task.h"
#include "uart-task/uart-task.h"

#define DEFAULT_PI_TERMS 1000000u

static bool parse_u32(const char *text, uint32_t *value)
{
    if (text == NULL ||
        !isdigit((unsigned char)text[0]))
        return false;

    char *end;
    errno = 0;

    unsigned long number = strtoul(text, &end, 10);

    if (*end != '\0' ||
        errno == ERANGE ||
        number > UINT32_MAX)
        return false;

    *value = (uint32_t)number;
    return true;
}

static void cmd_info(const command_t *command)
{
    (void)command;

    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);

    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);

    printf("chip: manufacturer 0x%03x, "
           "part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer,
           (unsigned)info.part,
           (unsigned)info.revision);

    printf("pico-sdk: %s\n", info.sdk_version);
}

static void cmd_uptime(const command_t *command)
{
    (void)command;

    printf("uptime: %llu ms\n",
           (unsigned long long)(systime_ms()));
}

static void cmd_profiling(const command_t *command)
{
    (void)command;

#if PROFILING_ENABLED

    for (uint32_t i = 0; i < PROFILING_STOPWATCH_COUNT; i++)
    {
        profiling_result_t result;

        if (profiling_get(i, &result))
        {
            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   result.name,
                   (unsigned)result.count,
                   result.mean_us,
                   (unsigned)result.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
}

static void led_reply(void)
{
    const char *state = "off";

    switch (led_task_get_state())
    {
    case LED_STATE_OFF:
        state = "off";
        break;

    case LED_STATE_ON:
        state = "on";
        break;

    case LED_STATE_BLINK:
        state = "blink";
        break;
    }

    printf("led: %s, period %u ms\n",
           state,
           (unsigned)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_ON);
    led_reply();
}

static void cmd_led_disable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_OFF);
    led_reply();
}

static void cmd_led_blink(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_BLINK);
    led_reply();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period;

    if (command->argc != 1 ||
        !parse_u32(command->argv[0], &period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (period == 0)
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    led_task_set_period_ms(period);
    led_reply();
}

static void cmd_button(const command_t *command)
{
    (void)command;

    printf("button: %s, presses %u\n",
           button_task_is_pressed()
               ? "pressed" : "released",
           (unsigned)button_task_get_press_count());
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = DEFAULT_PI_TERMS;

    if (command->argc > 1 ||
        (command->argc == 1 &&
         !parse_u32(command->argv[0], &terms)))
    {
        printf("error: usage pi_start [<terms>]\n");
        return;
    }

    if (terms == 0)
    {
        printf("error: terms must be greater than 0\n");
        return;
    }

    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }

    printf("pi: started, %u terms\n",
           (unsigned)terms);
}

static void cmd_pi(const command_t *command)
{
    (void)command;

    switch (pi_task_get_state())
    {
    case PI_NOT_STARTED:
        printf("pi: not started\n");
        break;

    case PI_RUNNING:
        printf("pi: running\n");
        break;

    case PI_READY:
    {
        double value;
        uint64_t elapsed_ms;

        if (pi_task_get_result(&value, &elapsed_ms))
        {
            printf("pi: %.9f in %llu ms\n",
                   value,
                   (unsigned long long)elapsed_ms);
        }
        break;
    }
    }
}


// Результат длительного измерения выдаётся здесь, не из задачи.
static void print_edge(platform_line_mode_t mode, bool ok,
                       uint32_t rise_us, uint32_t fall_us)
{
    const char *name = mode == PLATFORM_LINE_PUSH_PULL ? "pp" : "od";
    if (!ok)
    {
        printf("edge %s: timeout\n", name);
        return;
    }
    printf("edge %s: rise %.1f ms, fall %.1f ms\n",
           name, rise_us / 1000.0, fall_us / 1000.0);
}

static void cmd_edge(const command_t *command)
{
    platform_line_mode_t mode;
    if (command->argc != 1)
    {
        printf("error: usage edge pp|od\n");
        return;
    }
    if (strcmp(command->argv[0], "pp") == 0)
        mode = PLATFORM_LINE_PUSH_PULL;
    else if (strcmp(command->argv[0], "od") == 0)
        mode = PLATFORM_LINE_OPEN_DRAIN;
    else
    {
        printf("error: usage edge pp|od\n");
        return;
    }

    if (!edge_task_start(mode, print_edge))
        printf("error: edge is busy\n");
}

// Вывод принятого: все непечатные символы заменяем точками.
static void print_bytes(const uint8_t *data, size_t size)
{
    for (size_t i=0; i<size; ++i)
        putchar(isprint((unsigned char)data[i]) ? data[i] : '.');
}

static void print_errors(uint32_t errors)
{
    printf("err:");
    if (errors == 0) { printf(" none\n"); return; }
    if (errors & PLATFORM_UART_FE) printf(" FE");
    if (errors & PLATFORM_UART_PE) printf(" PE");
    if (errors & PLATFORM_UART_BE) printf(" BE");
    if (errors & PLATFORM_UART_OE) printf(" OE");
    putchar('\n');
}

static void cmd_loop(const command_t *command)
{
    if (command->argc != 1 ||
        (strcmp(command->argv[0], "internal") && strcmp(command->argv[0], "pins"))) {
        printf("error: usage loop internal|pins\n"); return;
    }
    bool internal = strcmp(command->argv[0], "internal") == 0;
    platform_uart_loop_set(internal);
    printf("loop: %s\n", internal ? "internal" : "pins");
}

static void print_echo(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("rx: ");
    print_bytes(data,size);
    putchar(' ');
    print_errors(errors);
}

#define ECHO_MAX 32
static void cmd_echo(const command_t *command)
{
    if (command->argc != 1 || command->argv[0][0] == 0) {
        printf("error: usage echo <text>\n"); return;
    }
    size_t size = strlen(command->argv[0]);
    if (size > ECHO_MAX) {
        printf("error: text longer than 32\n"); return;
    }
    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART0,
                         (const uint8_t *)command->argv[0], size, print_echo))
        printf("error: uart is busy\n");
}

const api_command_t api_commands[] = {
    { "info", "device passport", cmd_info },
    { "uptime", "time since reset", cmd_uptime },
    { "profiling", "show timing statistics", cmd_profiling },
    { "profiling_reset", "reset timing statistics", cmd_profiling_reset },
    { "led_enable", "turn LED on", cmd_led_enable },
    { "led_disable", "turn LED off", cmd_led_disable },
    { "led_blink", "blink LED", cmd_led_blink },
    { "led_period", "set LED period", cmd_led_period },
    { "button", "button state and press count", cmd_button },
    { "edge", "rise and fall of the RC line: edge pp|od", cmd_edge },
    { "loop", "UART0 loopback: loop internal|pins", cmd_loop },
    { "echo", "text through UART0 and back: echo <text>", cmd_echo },
    { "pi_start", "start PI calculation", cmd_pi_start },
    { "pi", "PI calculation status or result", cmd_pi }
};

const uint32_t api_command_count =
    sizeof(api_commands) / sizeof(api_commands[0]);
