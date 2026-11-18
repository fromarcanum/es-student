#include "button-task/button-task.h"

#include <stddef.h>

#include "platform/platform.h"
#include "systime/systime.h"

#define DEBOUNCE_US (20u * 1000u)
#define LONG_PRESS_US 1000000

typedef enum
{
    BUTTON_RELEASED,
    BUTTON_PRESS_PENDING,
    BUTTON_PRESSED,
    BUTTON_RELEASE_PENDING
} button_state_t;

typedef struct
{
    button_state_t state;
    uint64_t transition_us;
    uint64_t pressed_since_us;
    uint32_t press_count;
    bool long_press_reported;
    button_task_callback_t on_press_callback;
    button_task_callback_t on_long_press_callback;
} button_instance_t;

// У каждой кнопки собственный автомат и собственное время удержания.
static button_instance_t buttons[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button,
                      button_task_callback_t on_press,
                      button_task_callback_t on_long_press)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
        return;

    button_instance_t *instance = &buttons[button];
    instance->state = BUTTON_RELEASED;
    instance->transition_us = systime_us();
    instance->pressed_since_us = 0;
    instance->press_count = 0;
    instance->long_press_reported = false;
    instance->on_press_callback = on_press;
    instance->on_long_press_callback = on_long_press;
}

bool button_task_is_pressed(platform_button_t button)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
        return false;

    button_state_t state = buttons[button].state;
    return state == BUTTON_PRESSED || state == BUTTON_RELEASE_PENDING;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
        return 0;

    return buttons[button].press_count;
}

void button_task_handle(void)
{
    uint64_t now = systime_us();

    for (unsigned i = 0; i < PLATFORM_BUTTON_COUNT; ++i)
    {
        platform_button_t button = (platform_button_t)i;
        button_instance_t *instance = &buttons[i];
        bool pressed = platform_button_read(button);

        switch (instance->state)
        {
        case BUTTON_RELEASED:
            if (pressed)
            {
                instance->transition_us = now;
                instance->state = BUTTON_PRESS_PENDING;
            }
            break;

        case BUTTON_PRESS_PENDING:
            if (!pressed)
            {
                instance->state = BUTTON_RELEASED;
            }
            else if (now - instance->transition_us >= DEBOUNCE_US)
            {
                instance->state = BUTTON_PRESSED;
                instance->pressed_since_us = now;
                instance->long_press_reported = false;
                instance->press_count++;

                // У кнопки без долгого действия событие наступает сразу.
                if (instance->on_long_press_callback == NULL &&
                    instance->on_press_callback != NULL)
                    instance->on_press_callback();
            }
            break;

        case BUTTON_PRESSED:
            if (!pressed)
            {
                instance->transition_us = now;
                instance->state = BUTTON_RELEASE_PENDING;
            }
            else if (instance->on_long_press_callback != NULL &&
                     !instance->long_press_reported &&
                     now - instance->pressed_since_us >= LONG_PRESS_US)
            {
                // Флаг выставляем до callback: событие строго однократное.
                instance->long_press_reported = true;
                instance->on_long_press_callback();
            }
            break;

        case BUTTON_RELEASE_PENDING:
            if (pressed)
            {
                instance->state = BUTTON_PRESSED;
            }
            else if (now - instance->transition_us >= DEBOUNCE_US)
            {
                instance->state = BUTTON_RELEASED;

                // При долгом действии короткое объявляется лишь при отпускании.
                if (instance->on_long_press_callback != NULL &&
                    !instance->long_press_reported &&
                    instance->on_press_callback != NULL)
                    instance->on_press_callback();
            }
            break;
        }
    }
}
