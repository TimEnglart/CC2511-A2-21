#ifndef DRV8825_H
#define DRV8825_H

#include <stdbool.h>

// DRV8825 Related Functions
// No Dependencies

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

// A4988 has different microsteps and possibly other things
#define A4988_DRIVER

#ifndef A4988_DRIVER
#define DRV_MIN_STEP 0.03125
#else
#define DRV_MIN_STEP 0.0625
#endif

typedef unsigned char uint8_t;
typedef long unsigned int uint32_t;

// Determine the Percentage of a step based on the active modes
double drv_determine_step(bool mode_0, bool mode_1, bool mode_2);

// Determine the mode basic mode requried based on the step you want to perform
// Returns an byte where bit 1 is mode_2, 2 is mode_1, 3 is mode_0, 4 is invalid step
uint8_t drv_determine_mode(double step);

// Determine the Actual step count based on the number of steps at a certain step
double drv_determine_distance(double step_amount, unsigned int n_steps);

// Determine the amount of steps required for a certain distance in the provided mode
uint32_t drv_step_amount(double distance, bool mode_0, bool mode_1, bool mode_2);
uint32_t drv_step_amount_masked(double distance, uint8_t mode_mask);


#endif // DRV8825_H
