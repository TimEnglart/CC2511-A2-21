cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

set(projname "Assignment_2")

project(${projname} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

#include(example_auto_set_url.cmake)

add_executable(${projname}
        main.c
        pico.c
        utils.c
        drv8825.c
        menu.c
        queue.c
        )

target_link_libraries(${projname} pico_stdlib hardware_pwm hardware_uart hardware_adc hardware_irq hardware_dma pico_multicore pico_stdio_usb)
pico_add_extra_outputs(${projname})

