# CC2511 - Assignment 2 - Group 2
## Patrick, Rusty, Tim

## Requirements
- Nodejs + Yarn
- GCC Arm Compiler
- Rasberry Pi Pico SDK
- Rasberry Pi Pico Picoprobe Software + Picoprobe VSCode Configuration

## How to Use:
- Using the `ppgen` python script provided generate a project then move these files in
- The Pico Program Requires the following modules as cmake dependencies
    - `pico_stdlib`
    - `hardware_uart`
    - `hardware_irq`
    - `pico_multicore` 
    - `pico_stdio_usb`
- Procced to Flash and Start the PICO
- Open the `feed_serial` folder in a terminal/cmd window
- Run `yarn` to install the required dependencies for the Serial Transmission
- Run `yarn start` to run the script and being sending data to the pico to draw

## Changing Shapes to Send to the PICO
1. Obtain an SVG of the Image/Shape you want to draw.
2. Go to the website: https://spotify.github.io/coordinator/ to get your images turned into coordinates
> This Website flatterns the SVG into a single array of points so for complex svg's you will need to separate each element into its own array
3. Add Each Array to the `feed_serial/predefined_shapes.js` file under a new object where each property is a SVG path (eg. The Coordinates generated above)
4. Export the new object by placing the object in the `module.exports` object inside `feed_serial/predefined_shapes.js`
5. Import the new Object inside the `feed_serial/index.js` and modify the variable `paths`
6. Running `yarn start` should yield the pico drawing the new image.
> If you want to see a preview of the image that will be drawn uncomment the line containing `// continue; // Skip the Serial Transmission so we can dump`. This will skip drawing on the pico and export the images points. Opening the HTML file `visualise.html` should show the points drawn inside your web browser


## File Overview
### drv8825.h & drv8825.c
These are Step and mode related calculations for the DRV8825

### main.c
Initialises the PICO and contains the Loops for Core 0 & Core 1
- Core 0 is used for:
    - Program Setup and Teardown
    - UART Interrupts
- Core 1 is used for:
    - Processing Enqueued Step Data
    - Driving X, Y, Z Stepper Motors and Spindle

### menu.h & menu.c
Controls the GUI state of the UART display
- Handles User & Machine Input

### pico.h & pico.c
The Meat of the Program.
- Conatins Functions that Handle the State of all the Steppers, Spindle, Step Queue, etc.
- Header Contains all of the Defintions for PICO GPIO Operations

### queue.h & queue.c
Simple Thread Safe Double Ended Queue Implementation
- Could be replaced by `pico_util/queue`, but was made for flexibility
- Used to Queue all the Steps that are sent, which are then processed the the second core

### terminal.h
Header File provided to us to change Terminal Elements
- Colour
- Position
- Clear

### utils.h
Contains Bit Logic Macros, was seperated so that `drv8825.h` doesn't need to require `pico.h`


## feed_serial File Overview

### dump.js
Contains the scaled point data from the previous run of the program

### index.js
Connects to the Pico, Processes the provided point data and scales it to the PICO

### predefined_images.js
Contains to the point data for multiple images

### serial.js
Serial related functions that are referenced inside `index.js`

### utils.js
Mathematic Functions to re-scale the points to a dimension

### visualise.html
Renders the Point data from `dump.js` in the web browser for user inspection