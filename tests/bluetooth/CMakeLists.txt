add_executable(bluetooth_test)

target_sources(bluetooth_test
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/bluetooth_test.c
)

target_link_libraries(bluetooth_test
    PRIVATE
        # List of libraries to link:
        bt_serial
        pico_stdio_usb
        pico_runtime
        pico_time
)

pico_add_extra_outputs(bluetooth_test)
