add_executable(block_io_test)

target_sources(block_io_test
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/block_io_test.c
)

target_link_libraries(block_io_test
    PRIVATE
        # List of libraries to link:
        block_io
)

pico_add_extra_outputs(block_io_test)
