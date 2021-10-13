#ifndef DRV8825_H
#define DRV8825_H

#include <stdbool.h>

// TODO: Implement all declared functions
//      - Refactor if necessary 

// Data Sheet: https://www.ti.com/lit/gpn/drv8825


/*
    Timing Requirements:                                            MIN	    MAX	    UNIT
    1	ƒSTEP	    Step frequency		                                    250	    kHz
    2	tWH(STEP)	Pulse duration, STEP high	                    1.9		        μs
    3	tWL(STEP)	Pulse duration, STEP low	                    1.9		        μs
    4	tSU(STEP)	Setup time, command before STEP rising	        650		        ns
    5	tH(STEP)	Hold time, command after STEP rising	        650		        ns
    6	tENBL	    Enable time, nENBL active to STEP	            650		        ns
    7	tWAKE	    Wakeup time, nSLEEP inactive high to            1.7	            ms
                    STEP input accepted		
*/

#define DRV_STEP_FREQ_SLEEP 5
#define DRV_STEP_DURATION 3
#define DRV_SETUP_TIME_DURATION 3

typedef unsigned char uint8_t;



void drv_set_enabled(uint8_t gpio_pin, bool enabled);
char drv_has_fault(uint8_t gpio_pin);
void drv_reset(uint8_t gpio_pin, bool reset_state);
void drv_sleep(uint8_t gpio_pin, bool sleep_state);

// Determine the Percentage of a step based on the active modes
float drv_determine_step(bool mode_0, bool mode_1, bool mode_2);
// Determine the Actual step count based on the number of steps at a certain step
float drv_determine_distance(float step_amount, unsigned int n_steps);

#endif // DRV8825_H
