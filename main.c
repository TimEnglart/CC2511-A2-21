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

/*
  NOTES

  Origin of the Stepper is assumed to 0,0,0 (Therefore the smallest value can be 0. Top Left Placement with max Z height) 
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
#include "terminal.h"

// Wait for Interrupt to process the step queue. Comment out define to just sleep
#define WAIT_FOR_INTERRUPT_CORE_1

// Foward Declaration so that main stays at the top
void thread_main(void);

// Core 1
bool stop_processing;

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

  while (true) {
    __wfi(); // Wait for Interrupt
    // Do All the Logic in the Interrupt as we are not using the main loop for anything else
  }

  // Reset Position of Steppers
  // Get the Amount of Whole steps to get back to origin. int cast should floor
  int x_steps = (int)pico_state.drv_x_location;
  int y_steps = (int)pico_state.drv_y_location;
  int z_steps = (int)pico_state.drv_z_location;

  // Seperate the steps into Z then X & Y so that when reseting it doesn't continue to engrave
  drv_queue_node_t reset_z = {
    .z_steps = z_steps,
    .z_dir = false,

    // Take full steps
    .mode_0 = 0,
    .mode_1 = 0,
    .mode_2 = 0
  };

  // Get the Remaining Z Microsteps
  float z_steps_remainder = pico_state.drv_z_location - z_steps;
  uint8_t z_remainder_modes = drv_determine_mode(z_steps_remainder);
  drv_queue_node_t reset_z_remainder = {
    .z_steps = drv_step_amount_masked(z_steps_remainder, z_remainder_modes),
    .z_dir = false,
    .mode_0 = GET_BIT_N(z_remainder_modes, 3),
    .mode_1 = GET_BIT_N(z_remainder_modes, 2),
    .mode_2 = GET_BIT_N(z_remainder_modes, 1)
  };


  drv_queue_node_t reset_x_y = {
    .x_steps = x_steps,
    .x_dir = false,
    .y_steps = y_steps,
    .y_dir = false,

    // Take full steps
    .mode_0 = 0,
    .mode_1 = 0,
    .mode_2 = 0
  };
  
  // Get Microsteps for X & Y
  // Could move these into 1 operation but I don't want to go through the effort to
  // get the lowest common step between them. 
  // (Well I guess I could just use the step that is the smallest and then multiply the steps by the division between the 2 step amounts of the remainder)
  // stepCountH = stepCountH * (stepAmountH / stepAmountL)
  // OR have split modes
  float x_steps_remainder = pico_state.drv_x_location - x_steps;
  uint8_t x_remainder_modes = drv_determine_mode(x_steps_remainder);
  drv_queue_node_t reset_x_remainder = {
    .x_steps = drv_step_amount_masked(x_steps_remainder, x_remainder_modes),
    .x_dir = false,
    .mode_0 = GET_BIT_N(x_remainder_modes, 3),
    .mode_1 = GET_BIT_N(x_remainder_modes, 2),
    .mode_2 = GET_BIT_N(x_remainder_modes, 1)
  };
  float y_steps_remainder = pico_state.drv_y_location - y_steps;
  uint8_t y_remainder_modes = drv_determine_mode(y_steps_remainder);
  drv_queue_node_t reset_y_remainder = {
    .y_steps = drv_step_amount_masked(y_steps_remainder, y_remainder_modes),
    .y_dir = false,
    .mode_0 = GET_BIT_N(y_remainder_modes, 3),
    .mode_1 = GET_BIT_N(y_remainder_modes, 2),
    .mode_2 = GET_BIT_N(y_remainder_modes, 1)
  };

  queue_push(&pico_state.step_queue, &reset_z);
  queue_push(&pico_state.step_queue, &reset_z_remainder);
  queue_push(&pico_state.step_queue, &reset_x_y);
  queue_push(&pico_state.step_queue, &reset_x_remainder);
  queue_push(&pico_state.step_queue, &reset_y_remainder);

  // Disable the Processing Core
  stop_processing = true;

  // Disable UART
  pico_uart_deinit();
  // Free Menu Resources
  release_menus();
}


void thread_main(void)
{
  while(!stop_processing)
  {
    #ifdef WAIT_FOR_INTERRUPT_CORE_1
    // Want this at the top so that we can process data before we exit
    __wfi(); // Should be fine to wait for interrupt as UART is how data is being delievered
    // If this is to become a problem we could probably implement a mock interrupt that just returns
    // Or just ignore and not be battery efficient.
    #else
      sleep_ms(1000);
    #endif

    while(!queue_is_empty(&pico_state.step_queue) && pico_state.step_queue.running)
    {
      // Setup Information needed for step
      uint32_t step_mask = 0;
      drv_queue_node_t node;
      queue_pop(&pico_state.step_queue, &node);

      if(!node.initialized)
      {
        // We have failed to get the data for the steps.
        continue;
      }         
     
      // TODO: Either put the gpio initialisations in a seperate function w/ the respective sleeps
      // or add them here

      // Get the Step Size
      float step_size = drv_determine_step(node.mode_0, node.mode_1, node.mode_2);

      // Enable Drivers
      drv_enable_driver(true);


      // Setup Step Directions. TODO: Add Sleeps if needed
      drv_set_direction(X, node.x_dir);
      drv_set_direction(Y, node.y_dir);
      drv_set_direction(Z, node.z_dir);

      // Setup Mode. TODO: Add Sleeps if needed
      drv_set_mode(node.mode_0, node.mode_1, node.mode_2);

      // Turn on Spindle
      enable_spindle(true);
      
      // Need to init these initialisers for the loop as they underflow if not assigned
      // Did not know that (actually undefined behaviour)
      // https://stackoverflow.com/questions/21152138/local-variable-initialized-to-zero-in-c
      for(uint32_t x = 0, y = 0, z = 0; x <= node.x_steps || y <= node.y_steps || z <= node.z_steps; x = y = ++z)
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
    // TODO: May need to raise the Z motor if we stop the spindle to prevent it from catching
    enable_spindle(false);
    
    // Should we do this?
    drv_enable_driver(false);
  }
}