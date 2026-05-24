cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(gt_arcade_control_center C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

# P1 Absolute Mouse + CDC Control
add_executable(gt_arcade_p1_absolute_mouse
    src/mouse_main.c
    src/mouse_usb_descriptors.c
)
target_compile_definitions(gt_arcade_p1_absolute_mouse PRIVATE PLAYER_ID=1)
target_include_directories(gt_arcade_p1_absolute_mouse PRIVATE src ${PICO_SDK_PATH}/lib/tinyusb/src ${PICO_SDK_PATH}/lib/tinyusb/hw ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp)
target_link_libraries(gt_arcade_p1_absolute_mouse
    pico_stdlib
    hardware_adc
    hardware_flash
    tinyusb_device
    tinyusb_board
)
pico_enable_stdio_usb(gt_arcade_p1_absolute_mouse 0)
pico_enable_stdio_uart(gt_arcade_p1_absolute_mouse 0)
pico_add_extra_outputs(gt_arcade_p1_absolute_mouse)

# P2 Absolute Mouse + CDC Control
add_executable(gt_arcade_p2_absolute_mouse
    src/mouse_main.c
    src/mouse_usb_descriptors.c
)
target_compile_definitions(gt_arcade_p2_absolute_mouse PRIVATE PLAYER_ID=2)
target_include_directories(gt_arcade_p2_absolute_mouse PRIVATE src ${PICO_SDK_PATH}/lib/tinyusb/src ${PICO_SDK_PATH}/lib/tinyusb/hw ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp)
target_link_libraries(gt_arcade_p2_absolute_mouse
    pico_stdlib
    hardware_adc
    hardware_flash
    tinyusb_device
    tinyusb_board
)
pico_enable_stdio_usb(gt_arcade_p2_absolute_mouse 0)
pico_enable_stdio_uart(gt_arcade_p2_absolute_mouse 0)
pico_add_extra_outputs(gt_arcade_p2_absolute_mouse)

# Button Keyboard Board + CDC Control
add_executable(gt_arcade_button_keyboard
    src/keyboard_main.c
    src/keyboard_usb_descriptors.c
)
target_compile_definitions(gt_arcade_button_keyboard PRIVATE BOARD_ID=3)
target_include_directories(gt_arcade_button_keyboard PRIVATE src ${PICO_SDK_PATH}/lib/tinyusb/src ${PICO_SDK_PATH}/lib/tinyusb/hw ${PICO_SDK_PATH}/lib/tinyusb/hw/bsp)
target_link_libraries(gt_arcade_button_keyboard
    pico_stdlib
    hardware_flash
    tinyusb_device
    tinyusb_board
)
pico_enable_stdio_usb(gt_arcade_button_keyboard 0)
pico_enable_stdio_uart(gt_arcade_button_keyboard 0)
pico_add_extra_outputs(gt_arcade_button_keyboard)
