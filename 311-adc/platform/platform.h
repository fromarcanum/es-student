#pragma once

#include <stdbool.h>

// Запуск USB и настройка GPIO и АЦП.
void platform_init(void);

// Установить состояние светодиода.
void platform_led_set(bool on);

// Получить логическое состояние кнопки:
// true — нажата, false — отпущена.
bool platform_button_read(void);

// Напряжение на входе АЦП GP26, в вольтах.
float platform_voltage_read(void);

// Температура встроенного датчика RP2040, градусы Цельсия.
float platform_temp_read(void);
