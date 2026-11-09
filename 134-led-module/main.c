#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "led.h"

const uint BUTTON_PIN = 15;
const uint DEBOUNCE_MS = 20;

bool get_button_debounce(uint pin)
{
    bool state = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    return state && gpio_get(pin);
}

void report_led(void)
{
    printf("led %s\n", led_is_on() ? "on" : "off");
}

void handle_command(int command)
{
    if (command == 'e')
    {
        led_set(true);
        report_led();
    }
    else if (command == 'd')
    {
        led_set(false);
        report_led();
    }
    else
    {
        printf("unknown command: %c\n", command);
    }
}

int main(void)
{
    stdio_init_all();
    led_init();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    bool previous = false;

    while (1)
    {
        bool current = get_button_debounce(BUTTON_PIN);

        if (previous && !current)
        {
            led_toggle();
            report_led();
        }

        previous = current;

        int command = getchar_timeout_us(0);

        if (command != PICO_ERROR_TIMEOUT)
        {
            handle_command(command);
        }
    }
}
