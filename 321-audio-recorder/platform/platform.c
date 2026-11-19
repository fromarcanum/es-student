#include "platform/platform.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

// Только platform знает номера выводов и SDK.
#define LED_GPIO PICO_DEFAULT_LED_PIN
#define BUTTON_GPIO 15u
#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// SDK хранит состояние таймера здесь, пока он активен.
static repeating_timer_t blink_timer;
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

// Обработчик и суперцикл разделяют состояние захвата (обмен через volatile).
typedef enum {
    ADC_FREE,
    ADC_CAPTURE,
    ADC_BLOCK,
} adc_user_t;

static volatile adc_user_t adc_user;
static repeating_timer_t capture_timer;
static uint16_t capture_samples[PLATFORM_CAPTURE_MAX];
static uint32_t capture_times[PLATFORM_CAPTURE_MAX];
static uint32_t capture_count;
static uint32_t capture_period_us;
static volatile uint32_t capture_index;
static volatile uint32_t capture_handler_us;
static bool capture_started;

// Обработчик вызывается будильником таймера, а не суперциклом.
// Здесь нет печати, ожидания или длительных вычислений.
static bool on_blink(repeating_timer_t *timer)
{
    (void)timer;
    blink_on = !blink_on;
    gpio_put(LED_GPIO, blink_on);
    blink_toggles++;
    return true;
}


// Каждое прерывание снимает один отсчёт. Печати и ожиданий здесь нет.
static bool on_capture(repeating_timer_t *timer)
{
    (void)timer;
    uint32_t start_us = time_us_32();
    uint32_t index = capture_index;

    capture_times[index] = start_us;
    capture_samples[index] = adc_read();
    bool keep_running = (index + 1u < capture_count);
    capture_handler_us += time_us_32() - start_us;
    capture_index = index + 1u;

    if (!keep_running)
        adc_user = ADC_FREE;
    return keep_running;
}


// АЦП выдаёт отсчёты с собственной частотой, DMA считывает FIFO по DREQ_ADC.
#define ADC_CLOCK_HZ 48000000.0f

static int block_channel;
static uint16_t block_samples[PLATFORM_BLOCK_MAX];
static volatile uint32_t block_start_us;
static volatile uint32_t block_end_us;
static volatile bool block_ready;

static void adc_free_running(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(ADC_CLOCK_HZ / PLATFORM_SAMPLE_RATE_HZ - 1.0f);
}

// Вернуть АЦП в режим разовых измерений (get_voltage, get_temp, telemetry).
static void adc_one_shot(void)
{
    adc_run(false);
    adc_fifo_drain();
    adc_fifo_setup(false, false, 0, false, false);
    adc_set_clkdiv(0);
    adc_select_input(ADC_CHANNEL);
}

// Прерывание возникает только после завершения переноса всего блока DMA.
// В обработчике нет printf, sleep и ожидания завершения DMA.
static void on_block_done(void)
{
    dma_channel_acknowledge_irq1(block_channel);
    block_end_us = time_us_32();
    adc_one_shot();
    block_ready = true;
    adc_user = ADC_FREE;
}

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);

    // DMA_IRQ_1 выделен блочному захвату; DMA_IRQ_0 пригодится потоку.
    block_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq1_enabled(block_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, on_block_done);
    irq_set_enabled(DMA_IRQ_1, true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_GPIO, on);
}

bool platform_button_read(void)
{
    // Кнопка замыкает GP15 на GND.
    return gpio_get(BUTTON_GPIO) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    float voltage = adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10 || period_ms > 60000)
        return false;

    platform_blink_stop();
    blink_toggles = 0;
    // Отрицательный период задаёт постоянную сетку с начала предыдущего вызова.
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);
    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(LED_GPIO, false);
    return blink_toggles;
}

// АЦП занят лишь пока аппаратный захват ещё выполняется.
bool platform_adc_busy(void)
{
    return adc_user != ADC_FREE;
}

bool platform_capture_start(uint32_t count, uint32_t period_us)
{
    if (adc_user != ADC_FREE || count == 0 ||
        count > PLATFORM_CAPTURE_MAX || period_us < 10)
        return false;

    adc_select_input(ADC_CHANNEL);
    capture_count = count;
    capture_period_us = period_us;
    capture_index = 0;
    capture_handler_us = 0;
    capture_started = false;
    adc_user = ADC_CAPTURE;

    // Минус означает шаг от предыдущего запланированного срабатывания.
    if (!add_repeating_timer_us(-(int64_t)period_us, on_capture, NULL, &capture_timer))
    {
        adc_user = ADC_FREE;
        return false;
    }

    capture_started = true;
    return true;
}

bool platform_capture_done(void)
{
    return capture_started && adc_user == ADC_FREE &&
           capture_index == capture_count;
}

void platform_capture_get(platform_capture_t *capture)
{
    if (capture == NULL)
        return;

    capture->samples = capture_samples;
    capture->count = 0;
    capture->period_us = 0;
    capture->min_interval_us = 0;
    capture->max_interval_us = 0;
    capture->handler_percent = 0.0f;

    if (!platform_capture_done())
        return;

    capture->count = capture_count;
    capture->period_us = capture_period_us;
    if (capture_count < 2)
        return;

    uint32_t min_us = UINT32_MAX;
    uint32_t max_us = 0;
    for (uint32_t i = 1; i < capture_count; ++i)
    {
        uint32_t interval = capture_times[i] - capture_times[i - 1];
        if (interval < min_us)
            min_us = interval;
        if (interval > max_us)
            max_us = interval;
    }

    capture->min_interval_us = min_us;
    capture->max_interval_us = max_us;

    uint32_t elapsed_us = capture_times[capture_count - 1] - capture_times[0];
    if (elapsed_us != 0)
        capture->handler_percent = 100.0f * (float)capture_handler_us / (float)elapsed_us;
}

// Блок данных записывает DMA, ядро в цикле отсчёты не копирует.
bool platform_block_start(uint32_t count)
{
    if (adc_user != ADC_FREE || count == 0 || count > PLATFORM_BLOCK_MAX)
        return false;

    block_ready = false;
    adc_user = ADC_BLOCK;
    block_end_us = 0;
    adc_free_running();

    dma_channel_config cfg = dma_channel_get_default_config(block_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    // true запускает DMA, и он ждёт запроса от ADC FIFO.
    dma_channel_configure(block_channel, &cfg,
                          block_samples, &adc_hw->fifo, count, true);
    block_start_us = time_us_32();
    adc_run(true);
    return true;
}

bool platform_block_done(void)
{
    return block_ready;
}

const uint16_t *platform_block_samples(void)
{
    return block_samples;
}

uint32_t platform_block_time_us(void)
{
    return block_end_us - block_start_us;
}
