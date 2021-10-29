#include "menu.h"
#include "pico.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "drv8825.h"

char pending_character_buffer[INPUT_BUFFER_SIZE];
int pending_character_buffer_index;

// Input Based Menu
bool input_ready;
char input_buffer[INPUT_BUFFER_SIZE];
int input_buffer_index;

// WASD Based Menu
struct menu_node *current_menu, *main_menu, *manual_draw_menu, *automated_draw_menu;

// This is the y index for additional text to be printed on (or larger) so that it doesn't overlap the menu text
int text_output_y;

void uart_irq_handler(void)
{
  while (uart_is_readable(PICO_UART_ID))
  {
    // Leave Interrupt if we are not on a valid menu
    if(!current_menu)
      return;

    char ch = uart_getc(PICO_UART_ID);

    // Let the Menu Handle Key Presses / Exit if it has handled the keypress
    if (current_menu->override_irq && current_menu->override_irq(ch))
    {
      return;
    }
      
    
    switch (ch)
    {
    // Navigation
    // Move Up
    case 'w': 
      if (current_menu->current_selection > 0)
      {
        current_menu->previous_selection = current_menu->current_selection;
        current_menu->current_selection--;
        update_selection();
      }
      break;
    // Move Down
    case 's': 
      if (current_menu->current_selection < current_menu->options_length - 1)
      {
        current_menu->previous_selection = current_menu->current_selection;
        current_menu->current_selection++;
        update_selection();
      }
      break;

    // Redraw Menu
    case 'r':
      draw_menu();
      break;

    // Backspace
    case '\b':
    case 0x7f:
      // Beware: This can go back to an undefined menu
      go_to_menu(current_menu->previous_menu);
      break;

    // Enter
    case ' ':
    case '\r':
    case '\n':
      // Run the Selection Function for the current Menu Option 
      if (current_menu->options_length && current_menu->options[current_menu->current_selection].on_select)
        current_menu->options[current_menu->current_selection].on_select();
      break;
    default: // On non-special key
      break;
    }
  }
}

void update_selection(void)
{
  // Clear Previous Selection
  if (current_menu->previous_selection != -1)
  {
    term_move_to(0, OPTIONS_START_Y + current_menu->previous_selection);
    term_erase_line();
    term_set_color(clrWhite, clrBlack);
    printf("%s", current_menu->options[current_menu->previous_selection].option_text);
  }

  // Redraw the new selection
  term_move_to(0, OPTIONS_START_Y + current_menu->current_selection);
  term_erase_line();
  term_set_color(clrBlack, clrYellow);
  printf("%s", current_menu->options[current_menu->current_selection].option_text);
}

void draw_menu(void)
{
  // Draw Program Title
  term_move_to(0, 0);
  term_erase_line();
  term_set_color(clrCyan, clrBlack);
  printf("CC2511 - Group 2 - Assignment 2");

  // Draw Controls
  term_move_to(0, text_output_y + 1);
  term_erase_line();
  term_set_color(clrGreen, clrBlack);
  printf("Controls: W - Up | S - Down | Backspace - Back | Enter/Space - Select");

  // write_debug("\nLength: %d\nText: %s\n", current_menu->options_length, current_menu->options[0].option_text);

  // Draw Options
  for (int i = 0; i < current_menu->options_length; i++)
  {
    // Print the Menu Options
    // Make Background For Selected Option Different

    // Move to Line and Clear
    term_move_to(0, OPTIONS_START_Y + i);
    term_erase_line();

    // Give Visible Interaction for Selected Option
    if (current_menu->current_selection == i)
      term_set_color(clrBlack, clrYellow);
    else
      term_set_color(clrWhite, clrBlack);

    // Output the Option Text
    printf("%s", current_menu->options[i].option_text);
  }
}

void draw_textbox(unsigned short x, unsigned short y)
{
  static unsigned short inital_length = 12; // Length of the Textbox

  // Move to the Selected x & y
  term_move_to(x, y);
  term_erase_line();
  term_set_color(clrWhite, clrWhite);

  // Using a White Background Draw Blank Characters to create an "input box"
  for (unsigned short i = 0; i < inital_length; i++)
  {
    term_move_to(x + i, y);
    printf(" ");
  }

  // Set Colors back to default and move the terminal cursor back to the start of the "input box"
  term_set_color(clrBlack, clrWhite);
  term_move_to(x, y);
}

void create_menu(struct menu_node *menu, const char *menu_name, struct menu_node *previous_menu, int options_length, struct menu_option *options)
{
  // Assign all properties of the struct to the given or default value
  menu->previous_menu = previous_menu;
  menu->menu_name = menu_name;
  menu->options_length = options_length;
  menu->current_selection = 0;
  menu->previous_selection = -1;
  menu->override_irq = 0;

  // Prevent GC on Options when removed from the stack
  menu->options = (struct menu_option *)malloc(sizeof(struct menu_option) * options_length);
  memcpy(menu->options, options, sizeof(struct menu_option) * options_length);

  // Set the Text Output Section based on the largest Menu... for now (could dynamically clear)
  if (text_output_y < options_length)
    text_output_y = options_length;
}

void go_to_menu(struct menu_node *menu)
{
  // Clear the Options, Not the Entire Menu

  // Set the pointer of the current menu to the provided menu
  struct menu_node *previous_menu = current_menu;
  current_menu = menu;

  if(menu)
  {  
    // Clear the Overflowed Options
    // eg. Menu 1 has 7 Options & Menu 2 has 5 Options. The 6th and 7th option will remain on Menu 2. Remove them
    for (int i = current_menu->options_length; i < previous_menu->options_length; i++)
    {
      term_move_to(0, OPTIONS_START_Y + i);
      term_erase_line();
    }

    draw_menu();
  }
  else term_cls();
}

void go_to_manual_draw(void)
{
  go_to_menu(manual_draw_menu);
}
void go_to_automated_draw(void)
{
  go_to_menu(automated_draw_menu);
}

// Handle Keypresses for manual drawing
char manual_draw_irq(char ch) 
{
  /*
    Menu Description:
    Allows the user to use the uart keyboard to step the motors
  */

  // Statically Allocate our scoped variables that our function relies on
  static double step_amount = 1;
  static uint8_t step_multiplier = 1;

  double step = step_amount * step_multiplier;

  switch (ch)
  {
  // Move the Y Axis
  case 'w':
    drv_append_position(0, step, 0);
    break;
  case 's':
    drv_append_position(0, -step, 0);
    break;
  
  // Move the X Axis
  case 'a':
    drv_append_position(-step, 0, 0);
    break;
  case 'd':
    drv_append_position(step, 0, 0);
    break;
  
  // Move the Z Axis
  case 'q':
    drv_append_position(0, 0, step);
    break;
  case 'e':
    drv_append_position(0, 0, -step);
    break;

  // Change Step Amounts
  case 'z':
    step_amount -= DRV_MIN_STEP;
    if(step_amount < DRV_MIN_STEP) step_amount = DRV_MIN_STEP;
    break;
  case 'x':
    step_amount += DRV_MIN_STEP;
    if(step_amount > 1) step_amount = 1;
    break;

  // Update Step Multiplier
  case 'c':
    step_multiplier += 1;
    if(step_multiplier > 10) step_multiplier = 10;
    break;
  case 'v':
    step_multiplier -= 1;
    if(step_multiplier < 1) step_multiplier = 1;
    break;
  
  // Change Spindle State
  case 't':
    enable_spindle(true);
    break;
  case 'y':
    enable_spindle(false);
    break;

  default:
    return 0;
  }
  // Draw Instructions
  term_move_to(0, text_output_y + 2);
  term_set_color(clrWhite, clrBlack);
  printf("Current Step Value: %f\nCurrent Step Multiplier: %d", step_amount, step_multiplier);
  print_pico_state();
  return 1;
}

// Handle Keypresses for automated drawing
char automated_draw_irq(char ch) 
{
  /*
    Menu Description:
    Allows for automated coordinates to be used for drawing or
    Pass Custom Coords to the queue
  */

  // Setup Static and Scoped Variables used to track menu state
  static char buffer[300]; // Character buffer. Basically our string version of the double provided
  static uint8_t buffer_index = 0; // The Index of the latest charatcer in the buffer
  static double coordinates[3]; // Coordinate Buffer. Index 0 = X, 1 = Y, 2 = Z
  static uint8_t coordinate_index = 0; // The Index of the latest coordinate in the buffer

  switch (ch)
  {
  case ';': // End of Coords
    coordinates[coordinate_index++] = atof(buffer); // Convert our buffer to a double (Should be Z as it is the final element)
    drv_go_to_position(coordinates[0], coordinates[1], coordinates[2]); // Add the New Set of coordinates to the proccessing queue
    
    // Reset All Buffer Data
    coordinate_index = 0;
    buffer_index = 0;
    buffer[buffer_index] = '\0';
    break;
  case ',': // End of Axis Coordinate
    coordinates[coordinate_index++] = atof(buffer); // Convert our buffer to a double (Should be X or Y as they are not the final element)

    // Reset String Buffer Data
    buffer_index = 0;
    buffer[buffer_index] = '\0';
    break;
  case '\n':
    // As the Script Tries to access the Menu From Main Menu. Wipe Data on newline
    buffer_index = 0;
    buffer[buffer_index] = '\0';
    break;
  case '\b':
  case 0x7f:
    // On backspace. Let User Escape menu if they selected it and wipe any data they provided
    coordinate_index = 0;
    coordinates[0] = coordinates[1] = coordinates[2] = 0;
    buffer_index = 0;
    buffer[buffer_index] = '\0';
    return 0;
  default:
    // Store Character & Increment the string buffer when a non special character is received
    buffer[buffer_index++] = ch;
    buffer[buffer_index] = '\0';
    break;
  }
  return 1;
}
void generate_menus(void)
{
  // Allocate Memory for all the menus
  main_menu = (struct menu_node *)malloc(sizeof(struct menu_node));
  manual_draw_menu = (struct menu_node *)malloc(sizeof(struct menu_node));
  automated_draw_menu = (struct menu_node *)malloc(sizeof(struct menu_node));

  // Create Options

  // Main Menu
  struct menu_option main_menu_options[] = {
    {
      .on_select = go_to_manual_draw,
      .option_text = "Manual Draw"
    },
    {
      .on_select = go_to_automated_draw,
      .option_text = "Automated Draw [SCRIPT ONLY]"
    }
  };

  create_menu(main_menu, "Main Menu", 0, LENGTH_OF_ARRAY(main_menu_options), main_menu_options);

  struct menu_option manual_draw_menu_options[] = {
    {
      .option_text = "Manual Draw Controls"
    },
    {
      .option_text = "[D] - X Axis Right (+)"
    },
    {
      .option_text = "[A] - X Axis Left (-)"
    },
    {
      .option_text = "[W] - Y Axis Up (+)"
    },
    {
      .option_text = "[S] - Y Axis Down (-)"
    },
    {
      .option_text = "[Q] - Z Axis Forwards (+)"
    },
    {
      .option_text = "[E] - Z Axis Backwards (-)"
    },
    {
      .option_text = "[Z] - Increase Step Value (+)"
    },
    {
      .option_text = "[X] - Decrease Step Value (-)"
    },
    {
      .option_text = "[C] - Increase Step Multiplier (+)"
    },
    {
      .option_text = "[V] - Decrease Step Multiplier (-)"
    },
    {
      .option_text = "[T] - Enable Spindle Motor (+)"
    },
    {
      .option_text = "[Y] - Disable Spindle Motor (-)"
    },
  };

  // Manual Draw Menu (Manual Movements)
  create_menu(manual_draw_menu, "Manual Draw", main_menu, LENGTH_OF_ARRAY(manual_draw_menu_options), manual_draw_menu_options);
  manual_draw_menu->override_irq = manual_draw_irq; // Custom UART IRQ Handler
  
  // Automated Draw Menu (Serial Coordinates)
  create_menu(automated_draw_menu, "Automated Draw", main_menu, 0, 0);
  automated_draw_menu->override_irq = automated_draw_irq;

  // Set The Current Menu to Main Menu
  current_menu = main_menu;
}

void release_menus(void)
{ 
  // As we have manually assigned memory using memset. Release that Memory
  free(manual_draw_menu->options);
  free(manual_draw_menu);

  free(automated_draw_menu->options);
  free(automated_draw_menu);

  free(main_menu->options);
  free(main_menu);
}

void write_debug(const char *format, ...)
{
  term_move_to(0, 26);
  term_erase_line();
  term_set_color(clrMagenta, clrBlack);

  va_list args;
  va_start(args, format);
  printf(format, args);
  va_end(args);
}

void print_pico_state(void)
{
  term_move_to(0, text_output_y + 5);
  term_set_color(clrWhite, clrBlack);
  term_erase_line();
  printf("X: %.5f | Y: %.5f | Z: %.5f", 
    pico_state.drv_x_location, 
    pico_state.drv_y_location, 
    pico_state.drv_z_location
  );

  term_move_to(0, text_output_y + 6);
  term_set_color(clrWhite, clrBlack);
  term_erase_line();
    printf("<X>: %.5f | <Y>: %.5f | <Z>: %.5f", 
    pico_state.drv_x_location_pending, 
    pico_state.drv_y_location_pending, 
    pico_state.drv_z_location_pending
  );

  term_move_to(0, text_output_y + 7);
  term_set_color(clrWhite, clrBlack);
  term_erase_line();
    printf("m_0: %d | m_1: %d | m_2: %d", 
    pico_state.mode_0, 
    pico_state.mode_1, 
    pico_state.mode_2
  );

  term_move_to(0, text_output_y + 8);
  term_set_color(clrWhite, clrBlack);
  term_erase_line();
    printf("x_dir: %d | y_dir: %d | z_dir: %d", 
    pico_state.drv_x_direction, 
    pico_state.drv_y_direction, 
    pico_state.drv_z_direction
  );

  term_move_to(0, text_output_y + 9);
  term_set_color(clrWhite, clrBlack);
  term_erase_line();
    printf("!drv!: %d | !spindle!: %d | queue: %d", 
    pico_state.drv_enabled, 
    pico_state.spindle_enabled,
    pico_state.step_queue.length
  );
}