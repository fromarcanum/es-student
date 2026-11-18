#pragma once

#include <stdbool.h>

// Число и идентификаторы кнопок задаёт аппаратный уровень.
typedef enum
{
    PLATFORM_BUTTON_1 = 0,
    PLATFORM_BUTTON_2,
    PLATFORM_BUTTON_COUNT
} platform_button_t;

void platform_init(void);
void platform_led_set(bool on);
// true, если выбранная кнопка замкнута на землю.
bool platform_button_read(platform_button_t button);
