#include "pico.h"
#include "drv8825.h"
#include <math.h>


PICO_STATE pico_state;

// Initialise the DEBUG PICO's UART Pins
void pico_uart_init(irq_handler_t handler)
{
    gpio_init_mask((1 << PICO_UART_RX) | (1 << PICO_UART_TX));
    uart_init(PICO_UART_ID, PICO_BAUD_RATE);
    
    gpio_set_dir(PICO_UART_RX, false);
    gpio_set_dir(PICO_UART_TX, true);
    
    gpio_set_function(PICO_UART_RX, GPIO_FUNC_UART);
    gpio_set_function(PICO_UART_TX, GPIO_FUNC_UART);
    
    uart_set_hw_flow(PICO_UART_ID, false, false);
    uart_set_format(PICO_UART_ID, PICO_DATA_BITS, PICO_STOP_BITS, PICO_PARITY);
    uart_set_fifo_enabled(PICO_UART_ID, false);
    
    irq_set_exclusive_handler(PICO_UART_IRQ, handler);
    irq_set_enabled(PICO_UART_IRQ, true);
    uart_set_irq_enables(PICO_UART_ID, true, false);
}

void pico_gpio_init(int n_pins, ...) 
{
    va_list pins;
    va_start(pins, n_pins);
    for (int i = 0; i < n_pins; i++)
        gpio_init(va_arg(pins, uint));
    va_end(pins);
}

void pico_uart_deinit(void)
{
    // Disable Pins too?
    irq_clear(PICO_UART_IRQ);
    uart_deinit(PICO_UART_ID);
}

void enable_spindle(bool enabled)
{
    gpio_put(SPINDLE_TOGGLE, enabled);
    if(enabled) // Only Allow Wind-up not wind down
        busy_wait_ms(200); // Wind up time. Busy wait required as we use this inside an iterrupt :(
    pico_state.spindle_enabled = enabled;
}
void drv_set_mode(bool mode_0, bool mode_1, bool mode_2)
{
    gpio_put(DRV_MODE_0, mode_0);
    sleep_us(2); // Setup Time + Hold Time
    pico_state.mode_0 = mode_0;
    gpio_put(DRV_MODE_1, mode_1);
    sleep_us(2); // Setup Time + Hold Time
    pico_state.mode_1 = mode_1;
    gpio_put(DRV_MODE_2, mode_2);
    sleep_us(2); // Setup Time + Hold Time
    pico_state.mode_2 = mode_2;
}
void drv_set_direction(DRV_DRIVER axis, bool direction)
{
    char pin = drv_get_axis_pin(axis);
    gpio_put(pin + 1, direction);
    sleep_us(2); // Setup Time + Hold Time

    switch (axis)
    {
    case X:
        pico_state.drv_x_direction = direction;
        break;
    case Y:
        pico_state.drv_y_direction = direction;
        break;
    case Z:
        pico_state.drv_z_direction = direction;
        break;
    default:
        break;
    }
}


char drv_get_axis_pin(DRV_DRIVER axis)
{
    switch(axis)
    {
        case X:
            return DRV_X_STEP;
        case Y:
            return DRV_Y_STEP;
        case Z:
            return DRV_Z_STEP;
        default:
            return -1;
    }
}

void drv_enable_driver(bool enabled)
{
    // As we have all our drivers running off a single trace we cannot individually init them
    /*
        SLEEP - 
            Logic high to enable device, logic low to enter low-power sleep mode. 
            Internal pulldown
        ENABLE - 
            Logic high to disable device outputs and indexer operation, logic low to enable. 
            Internal pulldown
        RESET - 
            active-low reset input initializes the indexer logic and disables the H-bridge outputs. 
            Internal pulldown.
    */

    if(pico_state.drv_enabled != enabled)
    {
        // Are Sleeps Required Between the Changes Based on Datasheet or just before you send a step signal?

        gpio_put(DRV_SLEEP, enabled); // Make the Device Awake.
        sleep_ms(2);
        gpio_put(DRV_ENABLE, !enabled); // Make Sure it is giving outputs
        sleep_us(1);
        gpio_put(DRV_RESET, enabled); // Enable HBridge Outputs
        pico_state.drv_enabled = enabled;
    }
}

void drv_append_position(double x, double y, double z)
{
    return drv_go_to_position(
        pico_state.drv_x_location_pending + x, 
        pico_state.drv_y_location_pending + y, 
        pico_state.drv_z_location_pending + z
    );
}

// NOTE: X, Y, Z should be absolute values here (not relative)
void drv_go_to_position(double x, double y, double z)
{
    // Handle Position Overflows
    // Check if new location is more than the defined MAX_STEPS
    if(x > DRV_X_MAX_STEPS) x = DRV_X_MAX_STEPS;
    if(y > DRV_Y_MAX_STEPS) y = DRV_Y_MAX_STEPS;
    if(z > DRV_Z_MAX_STEPS) z = DRV_Z_MAX_STEPS;

    // Handle Position Underflow
    // Check if new location less than our set minimum
    if(x < DRV_X_MIN_STEPS) x = DRV_X_MIN_STEPS;
    if(y < DRV_Y_MIN_STEPS) y = DRV_Y_MIN_STEPS;
    if(z < DRV_Z_MIN_STEPS) z = DRV_Z_MIN_STEPS;

    // For Each Axis Determine the Direction
    bool x_dir = pico_state.drv_x_location_pending <= x,
        y_dir = pico_state.drv_y_location_pending <= y,
        z_dir = pico_state.drv_z_location_pending <= z;

    // For Each Axis Determine the Distance Required
    // As we have calculated the direction 
    // get the absolute distance between the current axis vs where we want to be
    double x_distance = fabs(pico_state.drv_x_location_pending - x),
        y_distance = fabs(pico_state.drv_y_location_pending - y),
        z_distance = fabs(pico_state.drv_z_location_pending - z);

    // For Each Axis Determine the Mode Required
    int8_t x_mode = drv_determine_mode(x_distance),
        y_mode = drv_determine_mode(y_distance),
        z_mode = drv_determine_mode(z_distance);

    // Get the Largest Mode (which is the smallest step) (As all motors share modes)
    uint8_t mode_mask = x_mode;
    if (y_mode > mode_mask) mode_mask = y_mode;
    if (z_mode > mode_mask) mode_mask = z_mode;

    // Validate that the provided position is within the allowed stepping range (as position is steps)
    // e.g. The new position is divisible by 0.03125 (32 microsteps)
    // Check the Error bit of the mode_mask
    if(GET_BIT_N(mode_mask, 3))
        return; // TODO: Find a better way to display this error

    // Update the pending locations
    pico_state.drv_x_location_pending = x;
    pico_state.drv_y_location_pending = y;
    pico_state.drv_z_location_pending = z;

    // Add Changes to the Queue
    drv_queue_node_t node = {
        .x_steps = drv_step_amount_masked(x_distance, mode_mask),
        .x_dir = x_dir,
        .y_steps = drv_step_amount_masked(y_distance, mode_mask),
        .y_dir = y_dir,
        .z_steps = drv_step_amount_masked(z_distance, mode_mask),
        .z_dir = z_dir,
        .mode_0 = GET_BIT_N(mode_mask, 2), 
        .mode_1 = GET_BIT_N(mode_mask, 1), 
        .mode_2 = GET_BIT_N(mode_mask, 0)
    };
    queue_push(&pico_state.step_queue, &node);
    
    // Send the GPIO Process Signal if we are using Iterrupts
    #ifdef WAIT_FOR_INTERRUPT_CORE_1
    gpio_put(PROCESS_QUEUE, GPIO_LOW);
    gpio_put(PROCESS_QUEUE, GPIO_HIGH);
    #endif
}