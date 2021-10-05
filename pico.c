#include "pico.h"
#include "hardware/pwm.h"

void pico_uart_init(irq_handler_t handler)
{
    uart_init(PICO_UART_ID, PICO_BAUD_RATE);

    gpio_init_mask((1 << PICO_UART_RX) | (1 << PICO_UART_TX));
    gpio_set_dir(PICO_UART_RX, false);
    gpio_set_dir(PICO_UART_TX, true);
    gpio_set_function(PICO_UART_RX, GPIO_FUNC_UART);
    gpio_set_function(PICO_UART_TX, GPIO_FUNC_UART);

    irq_set_exclusive_handler(PICO_UART_IRQ, handler);
    irq_set_enabled(PICO_UART_IRQ, true);
    uart_set_irq_enables(PICO_UART_ID, true, true);
}


void pico_pwm_init(uint gpio, uint pwm_wrap) {
    gpio_init(gpio);
    gpio_set_dir(gpio, true);
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint pwm_slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(pwm_slice, pwm_wrap);
    pwm_set_enabled(pwm_slice, 1);
}

void pico_gpio_init(int n_pins, ...) {
    va_list pins;
    va_start(pins, n_pins);
    for (int i = 0; i < n_pins; i++)
        gpio_init(va_arg(pins, uint));
    va_end(pins);
}

void pcio_uart_deinit(void)
{
    // Disable Pins too?
    irq_clear(PICO_UART_IRQ);
    uart_deinit(PICO_UART_ID);
}
