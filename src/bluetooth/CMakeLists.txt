add_library(bt_serial)

target_sources(bt_serial
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/bt_serial.c
    PUBLIC
        # List of public header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/bt_serial.h
)

target_include_directories(bt_serial
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bt_serial
    PUBLIC
        # List of libraries to link:
        hardware_uart
        hardware_irq
        hardware_gpio
        pico_util
)