#ifndef PICO_H
#define PICO_H

#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define PICO_UART_TX         0
#define PICO_UART_RX         1
#define PICO_UART_ID         uart0
#define PICO_UART_IRQ        UART0_IRQ
#define PICO_BAUD_RATE       115200
#define PICO_DATA_BITS       8
#define PICO_STOP_BITS       1
#define PICO_PARITY          UART_PARITY_NONE


#define DRV_MODE_2           2
#define DRV_MODE_1           3
#define DRV_MODE_0           4

#define DRV_DECAY            6
#define DRV_REST             7
#define DRV_SLEEP            8
#define DRV_ENABLE           9

#define DRV_X_STEP          10
#define DRV_X_DIRECTION     11

#define DRV_Y_STEP          12
#define DRV_Y_DIRECTION     13

#define DRV_Z_STEP          14
#define DRV_Z_DIRECTION     15


#define SPINDLE_TOGGLE      16


#define GPIO_OUTPUT_PINS \
    (1 << DRV_REST)         |\
    (1 << DRV_SLEEP)        |\
    (1 << DRV_DECAY)        |\
    (1 << DRV_ENABLE)       |\
                             \
    (1 << DRV_MODE_0)       |\
    (1 << DRV_MODE_1)       |\
    (1 << DRV_MODE_2)       |\
                             \
    (1 << DRV_X_DIRECTION)  |\
    (1 << DRV_X_STEP)       |\
    (1 << DRV_Y_DIRECTION)  |\
    (1 << DRV_Y_STEP)       |\
    (1 << DRV_Z_DIRECTION)  |\
    (1 << DRV_Z_STEP)       |\
                             \
    (1 << SPINDLE_TOGGLE)

#define GPIO_INPUT_PINS 0


void pico_pwm_init(uint gpio, uint pwm_wrap);

void pico_uart_init(irq_handler_t handler);

void pico_gpio_init(int n_pins, ...);

void pcio_uart_deinit(void);




#endif // PICO_H
