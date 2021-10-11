#include "drv8825.h"
#include "pico.h"

void drv_set_enabled(uint8_t gpio_pin, bool enabled)
{
    sleep_us(1);
}

float drv_determine_step(bool mode_0, bool mode_1, bool mode_2)
{
    if(!mode_0 && !mode_1 && !mode_2) 
        return 1; // Full step (2-phase excitation) with 71% current
    else if(!mode_0 && !mode_1 && mode_2)
        return 0.5; // 1/2 step (1-2 phase excitation)
    else if(!mode_0 && mode_1 && !mode_2)
        return 0.25; // 1/4 step (W1-2 phase excitation)
    else if(!mode_0 && mode_1 && mode_2)
        return 0.125; // 8 microsteps/step
    else if(mode_0 && !mode_1 && !mode_2)
        return 0.0625; // 16 microsteps/step
    else
        return 0.03125; // 32 microsteps/step
}

float drv_determine_distance(float step_amount, unsigned int n_steps)
{
    return step_amount * n_steps;
}