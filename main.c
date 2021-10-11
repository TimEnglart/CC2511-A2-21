/**************************************************************
 * main.c
 * Group 2
 * Assignment_2
 * ***********************************************************/

/*

TODO LIST

- Setup Codebase for DRV8825 based on Data Sheet (https://www.ti.com/lit/gpn/drv8825)
- Create Menu Navigation Framework
- Implement Required Functionality
- Create Variables to track the state of the motors
- Load Instructions to automate milling
- Document and Clean Code for more marks.

*/


#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico.h"
#include "menu.h"
#include <pthread.h>
#include "drv8825.h"
#include "queue.h"
#include "pico/multicore.h"

void thread_main(void)
{
  while(true)
  {
    while(!queue_is_empty(&pico_state.step_queue) && pico_state.step_queue.running)
    {
      // Setup Information needed for step
      uint32_t step_mask;
      drv_queue_node_t node;
      queue_pop(&pico_state.step_queue, &node);

      if(!node.initialized)
      {
        // We have failed to get the data for the steps.
        continue;
      }         
     
      // TODO: Either put the gpio initialisations in a seperate function w/ the respective sleeps
      // or add them here

      // Enable Drivers
      drv_enable_driver(true);


      // Setup Step Directions
      gpio_put(DRV_X_DIRECTION, node.x_dir);
      gpio_put(DRV_Y_DIRECTION, node.y_dir);
      gpio_put(DRV_Z_DIRECTION, node.z_dir);

      // Setup Mode
      float step_size = drv_determine_step(node.mode_0, node.mode_1, node.mode_2);
      gpio_put(DRV_MODE_0, node.mode_0);
      gpio_put(DRV_MODE_1, node.mode_1);
      gpio_put(DRV_MODE_2, node.mode_2);

      // Turn on Spindle
      gpio_put(SPINDLE_TOGGLE, true);

      for(uint32_t x, y, z; x <= node.x_steps || y <= node.y_steps || z <= node.z_steps; x = y = ++z)
      {
        // Setup Mask for this step
        SET_BIT_N(step_mask, DRV_X_STEP, x <= node.x_steps);
        SET_BIT_N(step_mask, DRV_Y_STEP, y <= node.y_steps);
        SET_BIT_N(step_mask, DRV_Z_STEP, z <= node.z_steps);

        // Step the Motors
        gpio_put_masked(step_mask, step_mask);
        sleep_us(3); // tWH(STEP)	Pulse duration, STEP high	1.9		μs (min)
        gpio_put_masked(step_mask, ~step_mask);
        sleep_us(3); // tWL(STEP)	Pulse duration, STEP low	1.9		μs (min)
        // The 2 Sleeps will generate our frequency (see figure 1. in Data Sheet)
        // As long as the overall time is larger than ~4us we are within the allowed frequency 


        // Update the State of the PICO's Step Counter
        if(GET_BIT_N(step_mask, DRV_X_STEP))
          pico_state.drv_x_location += (node.x_dir ? 1 : -1) * step_size;
        if(GET_BIT_N(step_mask, DRV_Y_STEP))
          pico_state.drv_y_location += (node.y_dir ? 1 : -1) * step_size;
        if(GET_BIT_N(step_mask, DRV_Z_STEP))
          pico_state.drv_z_location += (node.z_dir ? 1 : -1) * step_size;
      }      
    }
    // The above While will be skipped if there isn't anything to do

    // Turn off Spindle
    gpio_put(SPINDLE_TOGGLE, false);
    
    // Should we do this?
    drv_enable_driver(false);

    __wfi(); // Should be fine to wait for interrupt as UART is how data is being delievered
    // If this is to become a problem we could probably implement a mock interrupt that just returns

  }
}


int main(void) {
  // Enable Standard I/O Functionality
  stdio_init_all();

  // Turn on required PICO GPIO pins for UART
  pico_uart_init(uart_irq_handler);

  // Turn on DRV, Spindle and Header GPIO Pins
  gpio_init_mask(GPIO_OUTPUT_PINS);
  gpio_init_mask(GPIO_INPUT_PINS);

  // Set the Direction of the Enabled GPIO Pins
  gpio_set_dir_masked(GPIO_OUTPUT_PINS, GPIO_OUTPUT_PINS); // Outputs
  gpio_set_dir_masked(GPIO_INPUT_PINS, ~GPIO_INPUT_PINS); // Inputs


  // Set Pullups/Pulldowns for pins
  // gpio_set_pulls(DRV_FAULT, 1, 0); //Logic Low when Fault Occurs, Pull Up. No Fault Trace hahaha... :(

  // Setup Step Handler
  queue_init(&pico_state.step_queue);
  multicore_launch_core1(thread_main);

  // Create GUI Menus
  generate_menus();
  // Print the Menu to Screen
  draw_menu();
  
  while (in_menu) {
    __wfi();
  }
  
  // Disable UART
  pico_uart_deinit();
  // Free Menu Resources
  release_menus();
}
