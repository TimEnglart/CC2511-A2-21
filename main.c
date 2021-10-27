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
#include "pico/multicore.h"
#include "pico.h"
#include "menu.h"
#include "drv8825.h"
#include "queue.h"
#include "terminal.h"

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

  // Turn on the PICO LED so we can know if we have power
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  gpio_put(PICO_DEFAULT_LED_PIN, GPIO_HIGH);

  // Set Pullups/Pulldowns for pins
  // gpio_set_pulls(DRV_FAULT, 1, 0); //Logic Low when Fault Occurs, Pull Up. No Fault Trace hahaha... :(

  // Setup Step Handler
  queue_init(&pico_state.step_queue);
  multicore_launch_core1(thread_main);

  // Create GUI Menus
  generate_menus();
  // Print the Menu to Screen
  draw_menu();

  // While we are in the menu's
  while (current_menu) {
    __wfi(); // Wait for Interrupt
    // Do All the Logic in the Interrupt as we are not using the main loop for anything else
  }

  // Reset Position of Steppers
  // Get the Amount of Whole steps to get back to origin. int cast should floor
  int x_steps = (int)pico_state.drv_x_location_pending;
  int y_steps = (int)pico_state.drv_y_location_pending;
  int z_steps = (int)pico_state.drv_z_location_pending;

  // NOTE: Could use drv_go_to_position but need to provide additional pico states which append does for us
  // Step the Z Axis Back to Origin / 0
  // Example: If we are at 73.25 Steps on the Z axis
  drv_append_position(0, 0, (double)(-z_steps)); // Will be -73
  drv_append_position(0, 0, -pico_state.drv_z_location_pending); // Will be -0.25 as the pending location should be updated

  // Step the X & Y Axis Back to Origin / 0
  drv_append_position((double)(-x_steps), (double)(-y_steps), 0);
  // The Steps are below 1 so even the lowest chaneg shouldn't matter
  drv_append_position(-pico_state.drv_x_location_pending, -pico_state.drv_y_location_pending, 0);

  pico_state.step_queue.processing = true; // bye thread safety
  
  // Disable the Processing Core
  stop_processing = true;

  // Disable UART
  pico_uart_deinit();
  // Free Menu Resources
  release_menus();

  while(pico_state.step_queue.processing)
  {
    sleep_ms(100);
  }
  term_cls();
  // Turn off LED as the board has exited the main loop
  gpio_put(PICO_DEFAULT_LED_PIN, GPIO_LOW);
}

#ifdef WAIT_FOR_INTERRUPT_CORE_1
void process(uint gpio, uint32_t events)
{
  // Using this function to get out of the low power mode

  /*
    An Alternative to this using RTC (which seems a bit more complex than what we need):
    https://github.com/raspberrypi/pico-extras/blob/master/src/rp2_common/pico_sleep/sleep.c
    https://forums.raspberrypi.com/viewtopic.php?t=304815
  */
}
#endif

void thread_main(void)
{
  #ifdef WAIT_FOR_INTERRUPT_CORE_1
  // Setup a GPIO Pin which we put a interrupt on so that we can escape low power mode
  gpio_init(PROCESS_QUEUE);
  gpio_set_dir(PROCESS_QUEUE, GPIO_OUT);
  gpio_pull_up(PROCESS_QUEUE);
  gpio_set_irq_enabled_with_callback(PROCESS_QUEUE, GPIO_IRQ_EDGE_FALL, true, &process);
  #endif

  while(!stop_processing)
  {
    #ifdef WAIT_FOR_INTERRUPT_CORE_1
    // Want this at the top so that we can process data before we exit
    // We have setup a interrupt on the PROCESS_QUEUE gpio pin 
    // That interrupt will allow us to break out of the low power mode
    __wfi(); 
    #else
    sleep_ms(1000);
    #endif

    // Turn off LED to show processing
    gpio_put(PICO_DEFAULT_LED_PIN, GPIO_LOW);

    // NOTE: Even though inside the process_step_queue there is a while loop checking the exact same condition
    // there are sleeps after exiting the while loop. So check here aswell to prevent missing new steps
    do
    {
      process_step_queue(); // Process the Steps in the Queue and Act Upon Them
    } while (!queue_is_empty(&pico_state.step_queue));
    
    // Set the PICO LED to high to siginify that we have processed the data
    gpio_put(PICO_DEFAULT_LED_PIN, GPIO_HIGH);
    pico_state.step_queue.processing = false;
  }
}