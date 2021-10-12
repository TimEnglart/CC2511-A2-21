#ifndef PICO_MENU_H
#define PICO_MENU_H

#include "hardware/uart.h"
#include "stdarg.h"
#include <stdbool.h>

#define INPUT_BUFFER_SIZE 100
#define OPTIONS_START_Y 4

#define LENGTH_OF_ARRAY(x)  (sizeof(x) / sizeof((x)[0]))

// Let other modules know whether a menu is being shown. Allows for exit of the program
extern char in_menu;
extern struct menu_node *current_menu;
extern void (*selected_function)(void);

// Structure to represent and scope a menu option
struct menu_option {
    const char *option_text; // The Text Displayed in the CLI to represent the Selectable Option
    void (*on_select)(void); // A Function that is Called when the option is selected
};

// Structure to represent and scope menu specific data
struct menu_node {
    struct menu_node *previous_menu; // The menu we were previously on; so we can go back
    const char *menu_name; // The Displayed Name of the Menu when shown
    struct menu_option *options; // Pointer to an array of options that the menu has
    char options_length; // The amount of options the menu has
    char current_selection; // the currently selected options index
    char previous_selection; // the previously selected option index (So we dont need to redraw the entire screen) 
    char (*override_irq)(unsigned char); // IRQ Override function which allows menu to handle its own keypresses. Return 0 if character not handled, non-zero if handled
};

// The UART IRQ Handler (duh). Handles the UART Inputs received.
void uart_irq_handler(void);

// Draws the Entire Menu (Title, Options, etc). Should be called on start
void draw_menu(void);

// Redraws the CLI based on the selection change
void update_selection(void);

// (Helper Function) Updates the currently displayed menu
void go_to_menu(struct menu_node *menu);

// (Helper Function) Initialises a menu with its default values
void create_menu(struct menu_node *menu, const char* menu_name, struct menu_node *previous_menu, int options_length, struct menu_option *options);

// Allocate the required resources for the menus and define menu's here
void generate_menus(void);

// Write an Output to debug section in the CLI
void write_debug(const char* format, ...);

// Free's all resources for the menu allocated on the heap
void release_menus(void);

#endif // PICO_MENU_H