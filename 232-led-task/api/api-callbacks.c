#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "pico/stdlib.h"
#include "device.h"
#include "profiling.h"
#include "led-task.h"
#include "api-commands.h"

#define DEFAULT_PI_TERMS 1000000u

static volatile double pi_result;

static bool parse_u32(const char *text, uint32_t *value)
{
    if (!isdigit((unsigned char)text[0]))
        return false;

    char *end;
    errno = 0;

    unsigned long number = strtoul(text, &end, 10);

    if (*end != '\0' || errno == ERANGE ||
        number > UINT32_MAX)
        return false;

    *value = (uint32_t)number;
    return true;
}

static void cmd_info(const command_t *command)
{
    (void)command;
    device_info();
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n",
           (unsigned long long)(time_us_64() / 1000));
}

static double calc_pi(uint32_t terms)
{
    double sum = 0.0;
    double sign = 1.0;

    for (uint32_t k = 0; k < terms; k++)
    {
        sum += sign / (2.0 * (double)k + 1.0);
        sign = -sign;
    }

    return 4.0 * sum;
}

static void cmd_calc_pi(const command_t *command)
{
    uint32_t terms = DEFAULT_PI_TERMS;

    if (command->argc > 1 ||
        (command->argc == 1 &&
         !parse_u32(command->argv[0], &terms)))
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }

    if (terms == 0)
    {
        printf("error: terms must be greater than 0\n");
        return;
    }

    uint64_t start = time_us_64();
    pi_result = calc_pi(terms);
    uint64_t elapsed = time_us_64() - start;

    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n",
           (unsigned long long)(elapsed / 1000));
}

static void cmd_main_time_exec(const command_t *command)
{
    (void)command;
    printf("iteration avg %.2f us, max %u us\n",
           profiling_avg_us(),
           (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *command)
{
    (void)command;
    profiling_reset_max();
    printf("max reset\n");
}

// Общий ответ всех четырёх команд светодиода.
static void led_reply(void)
{
    const char *state = "off";

    switch (led_task_get_state())
    {
    case LED_STATE_OFF:   state = "off";   break;
    case LED_STATE_ON:    state = "on";    break;
    case LED_STATE_BLINK: state = "blink"; break;
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

    if (!led_task_set_period_ms(period))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    led_reply();
}

const api_command_t api_commands[] = {
    { "info", "device passport", cmd_info },
    { "uptime", "time since reset", cmd_uptime },
    { "calc_pi", "calculate pi: calc_pi [terms]", cmd_calc_pi },
    { "main_time_exec", "superloop iteration", cmd_main_time_exec },
    { "main_time_reset", "reset maximum iteration", cmd_main_time_reset },
    { "led_enable", "turn LED on", cmd_led_enable },
    { "led_disable", "turn LED off", cmd_led_disable },
    { "led_blink", "blink LED", cmd_led_blink },
    { "led_period", "set LED period: led_period <period_ms>", cmd_led_period }
};

const uint32_t api_command_count =
    sizeof(api_commands) / sizeof(api_commands[0]);
