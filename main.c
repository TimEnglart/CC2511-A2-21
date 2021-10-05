/**************************************************************
 * main.c
 * rev 1.0 03-Sep-2021 jc493462
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




int main(void) {
  stdio_init_all();

  // Turn on required PICO GPIO pins for UART
  pico_uart_init(uart_irq_handler);

  // Turn on Othe GPIO Pins
  gpio_init_mask(GPIO_OUTPUT_PINS);
  gpio_init_mask(GPIO_INPUT_PINS);

  // Set required GPIO pins to output
  gpio_set_dir_mask(GPIO_OUTPUT_PINS, GPIO_OUTPUT_PINS);
  

  // Inputs (None Requried ATM. They are on headers)
  gpio_set_dir_mask(GPIO_INPUT_PINS, GPIO_INPUT_PINS);

  // Create GUI Menus
  generate_menus();
  draw_menu();
  
  while (in_menu) {
    __asm("wfi");
  }
  
  // Disable UART
  pcio_uart_deinit();
  // Free Menu Resources
  release_menus();
}
