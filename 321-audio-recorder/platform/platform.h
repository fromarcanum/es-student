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

// Захват ADC0 (GP26) по периодическому будильнику.
#define PLATFORM_CAPTURE_MAX 2000

bool platform_adc_busy(void);
bool platform_capture_start(uint32_t count, uint32_t period_us);
bool platform_capture_done(void);

typedef struct {
    const uint16_t *samples;
    uint32_t count;
    uint32_t period_us;
    uint32_t min_interval_us;
    uint32_t max_interval_us;
    float handler_percent;
} platform_capture_t;

void platform_capture_get(platform_capture_t *capture);
