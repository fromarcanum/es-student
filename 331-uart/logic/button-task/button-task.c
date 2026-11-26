#include "button-task/button-task.h"

#include <stddef.h>

#include "platform/platform.h"
#include "systime/systime.h"

#define DEBOUNCE_US (20 * 1000)

typedef enum
{
    BUTTON_RELEASED,
    BUTTON_PRESS_PENDING,
    BUTTON_PRESSED,
    BUTTON_RELEASE_PENDING
} button_state_t;

static button_state_t state;
static uint64_t transition_us;
static uint32_t press_count;

static button_task_callback_t on_press_callback;

void button_task_init(button_task_callback_t on_press)
{
    state = BUTTON_RELEASED;
    transition_us = systime_us();
    press_count = 0;
    on_press_callback = on_press;
}

bool button_task_is_pressed(void)
{
    return state == BUTTON_PRESSED ||
           state == BUTTON_RELEASE_PENDING;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}

void button_task_handle(void)
{
    bool pressed = platform_button_read();
    uint64_t now = systime_us();

    switch (state)
    {
    case BUTTON_RELEASED:
        if (pressed)
        {
            transition_us = now;
            state = BUTTON_PRESS_PENDING;
        }
        break;

    case BUTTON_PRESS_PENDING:
        if (!pressed)
        {
            state = BUTTON_RELEASED;
        }
        else if (now - transition_us >= DEBOUNCE_US)
        {
            state = BUTTON_PRESSED;
            press_count++;

            if (on_press_callback != NULL)
                on_press_callback();
        }
        break;

    case BUTTON_PRESSED:
        if (!pressed)
        {
            transition_us = now;
            state = BUTTON_RELEASE_PENDING;
        }
        break;

    case BUTTON_RELEASE_PENDING:
        if (pressed)
        {
            state = BUTTON_PRESSED;
        }
        else if (now - transition_us >= DEBOUNCE_US)
        {
            state = BUTTON_RELEASED;
        }
        break;
    }
}
