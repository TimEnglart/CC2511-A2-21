#include "drv8825.h"
#include <math.h>
#include "utils.h"

double drv_determine_step(bool mode_0, bool mode_1, bool mode_2)
{
    // NOTE: Refer to the Step table in the DRV8825 Datasheet to why these values are being used
    #ifndef A4988_DRIVER
    // DRV8825 Steps
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
    #endif

    #ifdef A4988_DRIVER
    // A4988 Steps
    if(!mode_0 && !mode_1 && !mode_2) 
        return 1; // Full step (2-phase excitation) with 71% current
    else if(mode_0 && !mode_1 && !mode_2)
        return 0.5; // 1/2 step (1-2 phase excitation)
    else if(!mode_0 && mode_1 && !mode_2)
        return 0.25; // 1/4 step (W1-2 phase excitation)
    else if(mode_0 && mode_1 && !mode_2)
        return 0.125; // 8 microsteps/step
    else
        return 0.0625; // 16 microsteps/step
    #endif
}
uint8_t drv_determine_mode(double step)
{
    // Get the largest microstep mode that has no remainder
    // Basically the inverse of drv_determine_step

    // bit 1 is mode_2, 
    // bit 2 is mode_1, 
    // bit 3 is mode_0, 
    // bit 4 is invalid step

    if(!fmod(step, 1))
        return 0b0; 
    else if(!fmod(step, 0.5))
        return 0b001;
    else if(!fmod(step, 0.25))
        return 0b010;
     else if(!fmod(step, 0.125))
        return 0b011;
    #ifndef A4988_DRIVER
    else if(!fmod(step, 0.0625))
        return 0b100;
    else if(!fmod(step, 0.03125))
        return 0b111;
    #else
    // A4988 only goes down to 1/16th of a step
    else if(!fmod(step, 0.0625))
        return 0b111;
    #endif
    
    return 0b1000; // No Valid Step Increment Found
}

double drv_determine_distance(double step_amount, unsigned int n_steps)
{
    return step_amount * n_steps;
}

uint32_t drv_step_amount(double distance, bool mode_0, bool mode_1, bool mode_2)
{
    // Count the number of steps at the given mode fit into the distance
    uint32_t steps = 0;
    double step_amount = drv_determine_step(mode_0, mode_1, mode_2);
    for(double step = 0; step < distance; step += step_amount)
        steps++;
    return steps;
}
uint32_t drv_step_amount_masked(double distance, uint8_t mode_mask)
{
    // Extracts the modes from a mask and uses the drv_step_amount above to return the step count
    return drv_step_amount(distance, 
        #ifndef A4988_DRIVER
        GET_BIT_N(mode_mask, 2), 
        GET_BIT_N(mode_mask, 1), 
        GET_BIT_N(mode_mask, 0)
        #else
        GET_BIT_N(mode_mask, 0), 
        GET_BIT_N(mode_mask, 1), 
        GET_BIT_N(mode_mask, 2)
        #endif
    );
}