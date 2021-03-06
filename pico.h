#ifndef PICO_H
#define PICO_H

#include <stdbool.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "queue.h"
#include "utils.h"

#define GPIO_HIGH   1
#define GPIO_LOW    0


// UART Settings
#define PICO_UART_ID         uart0
#define PICO_UART_IRQ        UART0_IRQ
#define PICO_BAUD_RATE       115200
#define PICO_DATA_BITS       8
#define PICO_STOP_BITS       1
#define PICO_PARITY          UART_PARITY_NONE

// PICO GPIO PINS
#define PICO_UART_TX         0
#define PICO_UART_RX         1

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

#define PROCESS_QUEUE       21

// Wait for Interrupt to process the step queue. Comment out define to just sleep
#define WAIT_FOR_INTERRUPT_CORE_1


// Maximum and Minimum Nuber of Steps available on each axis (from a arbitray origin)
// TODO: Get the Max Steps Possible for each axis (Need to Physically Test the motors)
#define DRV_X_MAX_STEPS 100
#define DRV_Y_MAX_STEPS 100
#define DRV_Z_MAX_STEPS 100

#define DRV_X_MIN_STEPS 0
#define DRV_Y_MIN_STEPS 0
#define DRV_Z_MIN_STEPS 0

// Mask of all the gpio pin w/ direction out
#define GPIO_OUTPUT_PINS \
    (1 << DRV_RESET)        |\
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

// Mask of all the gpio pin w/ direction in
#define GPIO_INPUT_PINS 0

   
typedef enum { X, Y, Z } DRV_DRIVER;

typedef struct {
    // The Current Location of the Axis in Microsteps
    double drv_x_location, drv_y_location, drv_z_location;

    // The Future absolute location based on upcoming movements
    double drv_x_location_pending, drv_y_location_pending, drv_z_location_pending;

    // The Currently Enabled Modes
    bool mode_0, mode_1, mode_2;

    // The state of the Spindle
    bool spindle_enabled;

    // The Queue that contains all of the future movements for the steppers
    drv_queue_t step_queue;

    // is the motor enabled
    bool drv_enabled;

    // Motor Direction
    bool drv_x_direction, drv_y_direction, drv_z_direction;

} PICO_STATE;

// The current state of the program
// Contains values that track changes during runtime
extern PICO_STATE pico_state;

// (Helper Function) Intitialise UART on the PICO.
void pico_uart_init(irq_handler_t handler);
// (Helper Function) Initialise Multiple Pins without needing a mask
void pico_gpio_init(int n_pins, ...);
// (Helper Function) Disable UART Functionality 
void pico_uart_deinit(void);

// Process the Step Queue (Blocking)
void process_step_queue(void);
// Toggle the Spindle Motor
void enable_spindle(bool enabled);
// Set the Mode for the DRV's
void drv_set_mode(bool mode_0, bool mode_1, bool mode_2);
// Set the Direction for a DRV
void drv_set_direction(DRV_DRIVER axis, bool direction);

// Finds the Optimal modes and Direction to get the specified absolute position
void drv_go_to_position(double x, double y, double z);
// Appends the Given values onto the existing position
void drv_append_position(double x, double y, double z);


// Enable All DRV Drivers
void drv_enable_driver(bool enabled);
// Returns the STEP gpio pin for the given axis (STEP + 1 For the Direction of the Axis)
char drv_get_axis_pin(DRV_DRIVER axis);

#endif // PICO_H
