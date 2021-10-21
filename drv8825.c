#include "drv8825.h"
#include "pico.h"
#include <math.h>

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
uint8_t drv_determine_mode(float step)
{
    // Get the largest microstep mode that has no remainder
    // Basically the inverse of drv_determine_step
    if(!fmodf(step, 1))
        return 0b0; 
    else if(!fmodf(step, 0.5))
        return 0b001;
    else if(!fmodf(step, 0.25))
        return 0b010;
     else if(!fmodf(step, 0.125))
        return 0b011;
    else if(!fmodf(step, 0.0625))
        return 0b100;
    else if(!fmodf(step, 0.03125))
        return 0b111;
}

float drv_determine_distance(float step_amount, unsigned int n_steps)
{
    return step_amount * n_steps;
}

int drv_step_amount(float distance, bool mode_0, bool mode_1, bool mode_2)
{
    int step_counter = 0;
    float step_amount = drv_determine_step(mode_0, mode_1, mode_2);
    for(float i = 0; i < distance; i += step_amount)
        step_counter++;
    return step_counter;
}
int drv_step_amount_masked(float distance, uint8_t mode_mask)
{
    return drv_step_amount(distance, 
        GET_BIT_N(mode_mask, 3), 
        GET_BIT_N(mode_mask, 2), 
        GET_BIT_N(mode_mask, 1)
    );
}