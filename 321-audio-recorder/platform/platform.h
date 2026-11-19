#pragma once

#include <stdbool.h>
#include <stdint.h>

void platform_init(void);
void platform_led_set(bool on);
bool platform_button_read(void);
float platform_voltage_read(void);
float platform_temp_read(void);

// Индикатор мигает по будильнику таймера, независимо от суперцикла.
// Разрешённый период: 10..60000 мс.
bool platform_blink_start(uint32_t period_ms);

// Останавливает мигание, гасит LED и возвращает число переключений.
uint32_t platform_blink_stop(void);
