#pragma once

#include <stdbool.h>

// Запуск USB и настройка GPIO.
void platform_init(void);

// Установить состояние светодиода.
void platform_led_set(bool on);

// Получить логическое состояние кнопки:
// true — нажата, false — отпущена.
bool platform_button_read(void);
