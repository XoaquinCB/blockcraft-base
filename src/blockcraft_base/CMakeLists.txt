add_executable(blockcraft_base)

target_sources(blockcraft_base
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/blockcraft_base.c
    PUBLIC
        # List of public header files:
)

target_link_libraries(blockcraft_base
    PRIVATE
        # List of libraries to link:
)

pico_add_extra_outputs(blockcraft_base)
