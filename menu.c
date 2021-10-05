#include "menu.h"
#include "pico.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char pending_character_buffer[INPUT_BUFFER_SIZE];
int pending_character_buffer_index;

// Input Based Menu
bool input_ready;
char input_buffer[INPUT_BUFFER_SIZE];
int input_buffer_index;

// WASD Based Menu
struct menu_node *current_menu, *main_menu, *secondary_menu;
int text_output_y;
char in_menu;

void uart_irq_handler(void)
{
  while (uart_is_readable(PICO_UART_ID))
  {
    uint8_t ch = uart_getc(PICO_UART_ID);

    // Let the Menu Handle Key Presses / Exit if it has handled the keypress
    if (current_menu->override_irq && current_menu->override_irq(ch))
      return;

    pending_character_buffer[pending_character_buffer_index++] = ch;
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
      if (current_menu->previous_menu)
        go_to_menu(current_menu->previous_menu);
      else
        in_menu = 0;
      break;

    // Enter
    case ' ':
    case '\r':
    case '\n':
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
  if (!in_menu)
    in_menu = 1;

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

  write_debug("\nLength: %d\nText: %s\n", current_menu->options_length, current_menu->options[0].option_text);

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
  menu->options = malloc(sizeof(struct menu_option) * options_length);
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
  draw_menu();
  // Clear the Overflowed Options
  // eg. Menu 1 has 7 Options & Menu 2 has 5 Options. The 6th and 7th option will remain on Menu 2. Remove them
  for (int i = current_menu->options_length; i < previous_menu->options_length; i++)
  {
    term_move_to(0, OPTIONS_START_Y + i);
    term_erase_line();
  }
}

void first_option(void)
{
  printf("Selected Option: %s", current_menu->options[current_menu->current_selection].option_text);
}
void go_to_second_menu(void)
{
  go_to_menu(secondary_menu);
}
void second_option(void)
{
  printf(":)");
}

void generate_menus(void)
{
  // Allocate Memory for all the menus
  main_menu = (struct menu_node *)malloc(sizeof(struct menu_node));
  secondary_menu = (struct menu_node *)malloc(sizeof(struct menu_node));

  // Create Options

  // Main Menu
  struct menu_option main_menu_options[] = {
      {.on_select = first_option,
       .option_text = "1st Option"},
      {.on_select = first_option,
       .option_text = "2nd Option"},
      {.on_select = go_to_second_menu,
       .option_text = "3rd Option"},
      {.on_select = first_option,
       .option_text = "4th Option"}};

  create_menu(main_menu, "Main Menu", 0, LENGTH_OF_ARRAY(main_menu_options), main_menu_options);

  // Seconday menu
  struct menu_option secondary_menu_options[] = {
      {.on_select = second_option,
       .option_text = "Secondary Option #1"},
       {.on_select = second_option,
       .option_text = "Secondary Option #2"}};

  create_menu(secondary_menu, "Secondary Menu", main_menu, LENGTH_OF_ARRAY(secondary_menu_options), secondary_menu_options);

  // Set The Current Menu to Main Menu
  current_menu = main_menu;
}

void release_menus(void)
{
  // Some how dynamically free all Items.
  free(main_menu->options);
  free(main_menu);
  free(secondary_menu->options);
  free(secondary_menu);
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