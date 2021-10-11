#ifndef PICO_H
#define PICO_H

#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <pthread.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "queue.h"

#define SET_BIT(bitfield, bit) bitfield |= 1UL << bit
#define CLEAR_BIT(bitfield, bit) bitfield &= ~(1UL << bit)
#define TOGGLE_BIT(bitfield, bit) bitfield ^= 1UL << bit;
#define SET_BIT_N(bitfield, n, value) bitfield = (bitfield & ~(1UL << n)) | (value << n)
#define GET_BIT_N(bitfield, n) (bitfield >> n) & 1U



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
#define DRV_RESET            7
#define DRV_SLEEP            8
#define DRV_ENABLE           9

#define DRV_X_STEP          10
#define DRV_X_DIRECTION     11

#define DRV_Y_STEP          12
#define DRV_Y_DIRECTION     13

#define DRV_Z_STEP          14
#define DRV_Z_DIRECTION     15


#define SPINDLE_TOGGLE      16

// Max Frequncy is 250kHz, use 200kHz
#define DRV_STEP_FREQUENCY  200000


#define GPIO_OUTPUT_PINS \
    (1 << DRV_RESET)         |\
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


typedef enum { X, Y, Z } DRV_DRIVER;

typedef struct {
    // The Current Location of the Axis
    unsigned int drv_x_location, drv_y_location, drv_z_location;

    drv_queue_t step_queue;

    /* I dont know if these should be used as they can be fetched via functions */

    // is the motor enabled
    bool drv_x_enabled, drv_y_enabled, drv_z_enabled;

    // Motor Direction
    bool drv_x_direction, drv_y_direction, drv_z_direction;

} PICO_STATE;

extern PICO_STATE pico_state;

// (Helper Function) Intitialise a Pin for PWM.
void pico_pwm_init(uint gpio, uint pwm_wrap);
// (Helper Function) Intitialise UART on the PICO.
void pico_uart_init(irq_handler_t handler);
// (Helper Function) Initialise Multiple Pins without needing a mask
void pico_gpio_init(int n_pins, ...);
// (Helper Function) Disable UART Functionality 
void pico_uart_deinit(void);


// Enable All DRV Drivers
void drv_enable_driver(bool enabled);
// Step the given axis driver by one increment, set direction
void drv_step_driver(DRV_DRIVER axis, bool direction);
// Setup the Driver (PWM) for the provided axis
void drv_setup_driver(DRV_DRIVER axis, irq_handler_t on_pwm_wrap);
// Returns the STEP gpio pin for the given axis (STEP + 1 For the Direction of the Axis)
char drv_get_axis_pin(DRV_DRIVER axis);

#endif // PICO_H
