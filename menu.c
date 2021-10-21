#include "menu.h"
#include "pico.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

char pending_character_buffer[INPUT_BUFFER_SIZE];
int pending_character_buffer_index;

// Input Based Menu
bool input_ready;
char input_buffer[INPUT_BUFFER_SIZE];
int input_buffer_index;

// WASD Based Menu
struct menu_node *current_menu, *main_menu, *free_draw_menu, *predefined_draw_menu, *debug_menu;
int text_output_y;

void uart_irq_handler(void)
{
  while (uart_is_readable(PICO_UART_ID))
  {
    if(!current_menu)
      return;

    char ch = uart_getc(PICO_UART_ID);

    // Let the Menu Handle Key Presses / Exit if it has handled the keypress
    if (current_menu->override_irq && current_menu->override_irq(ch))
      return;
    
    switch (ch)
    {
    // Navigation
    case 'w':
      if (current_menu->current_selection > 0)
      {
        current_menu->previous_selection = current_menu->current_selection;
        current_menu->current_selection--;
        update_selection();
      }
      break;
    case 's':
      if (current_menu->current_selection < current_menu->options_length - 1)
      {
        current_menu->previous_selection = current_menu->current_selection;
        current_menu->current_selection++;
        update_selection();
      }
      break;

    // Use these for free-movement :)
    case 'a':
      break;

    case 'd':
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
      if (current_menu->options[current_menu->current_selection].on_select)
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
  term_move_to(0, 20);
  term_erase_line();
  term_set_color(clrGreen, clrBlack);
  printf("Controls: W - Up | S - Down | A - Left | D - Right");

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
  static unsigned short inital_length = 12;

  term_move_to(x, y);
  term_erase_line();
  term_set_color(clrWhite, clrWhite);
  for (unsigned short i = 0; i < inital_length; i++)
  {
    term_move_to(x + i, y);
    printf(" ");
  }
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

void go_to_free_draw(void)
{
  go_to_menu(free_draw_menu);
}
void go_to_predefined_draw(void)
{
  go_to_menu(predefined_draw_menu);
}
void go_to_debug(void)
{
  go_to_menu(debug_menu);
}

// Handle Keypresses for free drawing
char free_draw_irq(char ch) 
{
  /*
    Menu Description:
    Allows the user to use the uart keyboard to step the motors
  */

  // Statically Allocate our scoped variables that our function relies on
  static int x, y, z;

  switch (ch)
  {
  case 'w':
    
    break;
  case 's':
    
    break;
  case 'a':
    
    break;
  case 'd':
    
    break;
  
  default:
    return 0;
  }
  return 1;
}

// Handle Keypresses for predefined drawing
char predefined_draw_irq(char ch) 
{
  /*
    Menu Description:
    Allows for predefined coordinates to be used for drawing or
    Pass Custom Coords to the queue
  */
  switch (ch)
  {
  case 'w':
    
    break;
  case 's':
    
    break;
  case 'a':
    
    break;
  case 'd':
    
    break;
  
  default:
    return 0;
  }
  return 1;
}

void debug_change(bool increment)
{
  switch (current_menu->current_selection)
  {
  case 0: // DRV_X_LOC
    break;
  case 1: // DRV_X_DIR
    break;
  case 2: // DRV_Y_LOC
    break;
  case 3: // DRV_Y_DIR
    break;
  case 4: // DRV_Z_LOC
    break;
  case 5: // DRV_Z_DIR
    break;
  case 6: // DRV_ENABLE
    break;
  case 7: // DRV_DECAY
    break;
  case 8: // DRV_RESET
    break;
  case 9: // DRV_SLEEP
    break;
  case 10: // MODE_0
    break;
  case 11: // MODE_1
    break;
  case 12: // MODE_2
    break;
  case 13: // SPINDLE_TOGGLE
    break;
  
  default:
    break;
  }
}
// Handle Keypresses on the debug menu 
char debug_irq(char ch) 
{
  /*
    Menu Description:
    This menu bascially allows the user to change many of the state variables within the program
  */

  switch (ch)
  {
  case 'a':
  case 'd':
    debug_change(ch == 'd');
    break;
  default:
    return 0;
  }
  return 1;
}

void generate_menus(void)
{
  // Allocate Memory for all the menus
  main_menu = (struct menu_node *)malloc(sizeof(struct menu_node));
  free_draw_menu = (struct menu_node *)malloc(sizeof(struct menu_node));
  predefined_draw_menu = (struct menu_node *)malloc(sizeof(struct menu_node));
  debug_menu = (struct menu_node *)malloc(sizeof(struct menu_node));

  // Create Options

  // Main Menu
  struct menu_option main_menu_options[] = {
    {
      .on_select = go_to_free_draw,
      .option_text = "Free Draw"
    },
    {
      .on_select = go_to_predefined_draw,
      .option_text = "Predefined Draw"
    },
    {
      .on_select = go_to_debug,
      .option_text = "Debug"
    }
  };

  create_menu(main_menu, "Main Menu", 0, LENGTH_OF_ARRAY(main_menu_options), main_menu_options);

  // Free Draw Menu
  create_menu(free_draw_menu, "Free Draw", main_menu, 0, 0);
  free_draw_menu->override_irq = free_draw_irq; // Custom UART IRQ Handler
  // Predefined Draw ()

  create_menu(predefined_draw_menu, "Predefined Draw", main_menu, 0, 0);
  predefined_draw_menu->override_irq = predefined_draw_irq;

  struct menu_option debug_menu_options[] = {
    {
      .option_text = "[-] DRV_X_LOCATION [+]"
    },
    {
      .option_text = "[-] DRV_X_DIRECTION [+]"
    },
    {
      .option_text = "[-] DRV_Y_LOCATION [+]"
    },
    {
      .option_text = "[-] DRV_Y_DIRECTION [+]"
    },
    {
      .option_text = "[-] DRV_Z_LOCATION [+]"
    },
    {
      .option_text = "[-] DRV_Z_DIRECTION [+]"
    },
    {
      .option_text = "[-] DRV_ENABLE [+]"
    },
    {
      .option_text = "[-] DRV_DECAY [+]"
    },
    {
      .option_text = "[-] DRV_RESET [+]"
    },
    {
      .option_text = "[-] DRV_SLEEP [+]"
    },
    {
      .option_text = "[-] DRV_MODE_0 [+]"
    },
    {
      .option_text = "[-] DRV_MODE_1 [+]"
    },
    {
      .option_text = "[-] DRV_MODE_2 [+]"
    },
    {
      .option_text = "[-] SPINDLE_STATUS [+]"
    }
  };
  create_menu(debug_menu, "Debug", main_menu, LENGTH_OF_ARRAY(debug_menu_options), debug_menu_options);
  debug_menu->override_irq = debug_irq;
  // Set The Current Menu to Main Menu
  current_menu = main_menu;
}

void release_menus(void)
{
  // Some how dynamically free all Items.
  free(debug_menu->options);
  free(debug_menu);
  
  free(free_draw_menu->options);
  free(free_draw_menu);

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