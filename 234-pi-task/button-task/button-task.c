#include "button-task.h"

#include "pico/stdlib.h"
#include "led-task.h"

#define BUTTON_PIN 15u
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

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    state = BUTTON_RELEASED;
    transition_us = time_us_64();
    press_count = 0;
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
    bool pressed = !gpio_get(BUTTON_PIN);
    uint64_t now = time_us_64();

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
            led_task_next_state();
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
