#include "platform/platform.h"
#include "pico/stdlib.h"

// Номера GPIO известны только аппаратному уровню.
#define LED_GPIO PICO_DEFAULT_LED_PIN
static const unsigned button_gpio[PLATFORM_BUTTON_COUNT] = {15u, 14u};

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    for (unsigned i = 0; i < PLATFORM_BUTTON_COUNT; ++i)
    {
        gpio_init(button_gpio[i]);
        gpio_set_dir(button_gpio[i], GPIO_IN);
        gpio_pull_up(button_gpio[i]);
    }
}

void platform_led_set(bool on)
{
    gpio_put(LED_GPIO, on);
}

bool platform_button_read(platform_button_t button)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
        return false;

    return gpio_get(button_gpio[button]) == 0;
}
