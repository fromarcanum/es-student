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

// 44 100 отсчётов АЦП в секунду, буфер из 8192 uint16_t.
#define PLATFORM_SAMPLE_RATE_HZ 44100
#define PLATFORM_BLOCK_MAX 8192

// Возвращает false при занятости АЦП или неверном числе отсчётов.
bool platform_block_start(uint32_t count);
bool platform_block_done(void);
const uint16_t *platform_block_samples(void);
uint32_t platform_block_time_us(void);

// Непрерывный поток: два буфера по 1024 байта, 16 бит/отсчёт.
#define PLATFORM_STREAM_BYTES 1024
bool platform_stream_start(uint8_t bits);
void platform_stream_stop(void);
const uint8_t *platform_stream_next(uint32_t *samples);
void platform_stream_release(void);
uint32_t platform_stream_overruns(void);
void platform_write(const void *data, uint32_t bytes);
