#include "platform/platform.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/regs/uart.h"
#include "hardware/irq.h"

// Только platform знает номера выводов.
#define LED_GPIO PICO_DEFAULT_LED_PIN
#define BUTTON_GPIO 15u

// Вход АЦП: вывод GP26, канал 0.
#define ADC_PIN 26
#define ADC_CHANNEL 0
#define LINE_PIN 22u
#define UART0_TX_PIN 0u
#define UART0_RX_PIN 1u
#define UART_BAUD 115200u
#define UART1_TX_PIN 8u
#define UART1_RX_PIN 9u
#define RX_BUFFER_SIZE 128
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint32_t rx_count;

static void on_uart0_rx(void)
{
    while (uart_is_readable(uart0)) {
        uint8_t byte = uart_get_hw(uart0)->dr & 0xffu;
        if (rx_count < RX_BUFFER_SIZE) rx_buffer[rx_count++] = byte;
    }
}


// Опорное напряжение и число уровней 12-разрядного АЦП.
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

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

    gpio_init(LINE_PIN);
    platform_line_set(PLATFORM_LINE_PUSH_PULL, false);

    uart_init(uart0, UART_BAUD);
    platform_uart_loop_set(false);
    uart_init(uart1, UART_BAUD);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);
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


// GP22 через 1 кОм подключён к измеряемому узлу GP26.
void platform_line_set(platform_line_mode_t mode, bool high)
{
    if (mode == PLATFORM_LINE_PUSH_PULL)
    {
        gpio_disable_pulls(LINE_PIN);
        gpio_put(LINE_PIN, high);
        gpio_set_dir(LINE_PIN, GPIO_OUT);
        return;
    }

    // В режиме открытого стока единица НЕ записывается в регистр вывода:
    // GP22 либо притягивает линию к GND, либо отпускает её.
    gpio_pull_up(LINE_PIN);
    gpio_put(LINE_PIN, 0);
    gpio_set_dir(LINE_PIN, high ? GPIO_IN : GPIO_OUT);
}

static uart_inst_t *uart_of(platform_uart_t uart)
{
    return uart == PLATFORM_UART0 ? uart0 : uart1;
}

void platform_uart_loop_set(bool internal)
{
    uint function = internal ? GPIO_FUNC_NULL : GPIO_FUNC_UART;
    gpio_set_function(UART0_TX_PIN, function);
    gpio_set_function(UART0_RX_PIN, function);
    if (internal) {
        hw_set_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    } else {
        hw_clear_bits(&uart_get_hw(uart0)->cr, UART_UARTCR_LBE_BITS);
    }
}

void platform_uart_clear(platform_uart_t uart)
{
    uart_inst_t *u = uart_of(uart);
    while (uart_is_readable(u))
        (void)uart_get_hw(u)->dr;
    uart_get_hw(u)->rsr = 0; // UARTECR: сброс ошибок
    if (uart == PLATFORM_UART0) rx_count = 0;
}

bool platform_uart_put(platform_uart_t uart, uint8_t byte)
{
    uart_inst_t *u = uart_of(uart);
    if (!uart_is_writable(u)) return false;
    uart_putc_raw(u, byte);
    return true;
}

bool platform_uart_busy(platform_uart_t uart)
{
    return (uart_get_hw(uart_of(uart))->fr & UART_UARTFR_BUSY_BITS) != 0;
}

size_t platform_uart_read(platform_uart_t uart, uint8_t *data, size_t size)
{
    uart_inst_t *u = uart_of(uart);
    size_t n = 0;
    if (uart == PLATFORM_UART0) {
        while (n < size && n < rx_count) {
            data[n] = rx_buffer[n];
            n++;
        }
        rx_count = 0;
    }
    while (n < size && uart_is_readable(u))
        data[n++] = uart_get_hw(u)->dr & 0xff;
    return n;
}

uint32_t platform_uart_errors(platform_uart_t uart)
{
    return uart_get_hw(uart_of(uart))->rsr & 0xfu;
}

uint32_t platform_uart_baud_set(platform_uart_t uart, uint32_t baud)
{
    return uart_set_baudrate(uart_of(uart), baud);
}

void platform_uart_parity_set(platform_uart_t uart, platform_parity_t parity)
{
    static const uart_parity_t parities[] = {
        UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD
    };
    uart_set_format(uart_of(uart), 8, 1, parities[parity]);
}

void platform_uart_rx_irq_set(bool on)
{
    uart_set_irqs_enabled(uart0, on, false);
}
