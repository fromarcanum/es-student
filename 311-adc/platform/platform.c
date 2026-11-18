#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"

// Только platform знает номера выводов.
#define LED_GPIO PICO_DEFAULT_LED_PIN
#define BUTTON_GPIO 15u

// Вход АЦП: вывод GP26, канал 0.
#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4

// Опорное напряжение и число уровней 12-разрядного АЦП.
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_GPIO, on);
}

bool platform_button_read(void)
{
    // Кнопка замыкает GP15 на GND.
    return gpio_get(BUTTON_GPIO) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

// Температура кристалла RP2040, градусы Цельсия.
float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    float voltage = adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}
