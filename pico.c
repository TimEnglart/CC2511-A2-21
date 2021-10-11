#include "pico.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

PICO_STATE pico_state;

// Initialise the DEBUG PICO's UART Pins
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

    // Are Sleeps Required Between the Changes Based on Datasheet or just before you send a step signal?
    gpio_put(DRV_SLEEP, enabled); // Make the Device Awake.
    sleep_ms(2);
    gpio_put(DRV_ENABLE, !enabled); // Make Sure it is giving outputs
    sleep_us(1);
    gpio_put(DRV_RESET, enabled); // Enable HBridge Outputs
}

void drv_step_driver(DRV_DRIVER axis, bool direction)
{
    char gpio_pin = drv_get_axis_pin(axis);
    if(gpio_pin < 0)
        return; // We have an invaild Axis Provided

    gpio_put(gpio_pin + 1, direction);

    // Turn on the PWM for the Axis... Should Disable itself after 1 PWM cycle
    pwm_set_enabled(pwm_gpio_to_slice_num(gpio_pin), 1);
}

// TOOD: Remove this later as we are not using pwm
void drv_setup_driver(DRV_DRIVER axis, irq_handler_t on_pwm_wrap)
{
    // https://forums.raspberrypi.com/viewtopic.php?t=303116

    char gpio_pin = drv_get_axis_pin(axis);
    if(gpio_pin < 0)
        return; // We have an invaild Axis Provided

    // Get PWM info
    uint slice_num = pwm_gpio_to_slice_num(gpio_pin);
    uint channel = pwm_gpio_to_channel(gpio_pin);

    // Config the PWM. Time for some big boy engineering math
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING); // Free Running Mode thanks
    // Setup Frequncy
    uint f_sys = clock_get_hz(clk_sys); // Get System Clock
	float divider = f_sys / 1000000UL; // Have Divider run at 1MHz
	pwm_config_set_clkdiv(&config, divider); // Set Divider
	uint top =  1000000UL / DRV_STEP_FREQUENCY - 1; // Wrap will be based on DRV_STEP_FREQUENCY
	pwm_set_wrap(slice_num, top);
    
    
    // Setup GPIO Function
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    // Setup IRQ
    // We have IRQ so that on every pwm wrap ("tick") at said frequency
    // we can move the motor if need be in a non blocking/busy waiting behaviour. 
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, 1);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, 1);
    // Enable PWM
    // 3rd param is to start pwm (automatic|manu)ally
    pwm_init(slice_num, &config, 0); // Not Started Automatically
}