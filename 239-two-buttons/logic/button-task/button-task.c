#include "button-task/button-task.h"

#include <stddef.h>

#include "platform/platform.h"
#include "systime/systime.h"

#define DEBOUNCE_US (20u * 1000u)

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
    uint32_t press_count;
    button_task_callback_t on_press_callback;
} button_instance_t;

// Два независимых автомата, одинаковый алгоритм и период подавления дребезга.
static button_instance_t buttons[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press)
{
    if ((unsigned)button >= PLATFORM_BUTTON_COUNT)
        return;

    button_instance_t *instance = &buttons[button];
    instance->state = BUTTON_RELEASED;
    instance->transition_us = systime_us();
    instance->press_count = 0;
    instance->on_press_callback = on_press;
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
                instance->press_count++;
                if (instance->on_press_callback != NULL)
                    instance->on_press_callback();
            }
            break;
        case BUTTON_PRESSED:
            if (!pressed)
            {
                instance->transition_us = now;
                instance->state = BUTTON_RELEASE_PENDING;
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
            }
            break;
        }
    }
}
