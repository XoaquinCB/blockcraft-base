add_library(block_io)

target_sources(block_io
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/block_io.c
    PUBLIC
        # List of public header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/block_io.h
)

target_include_directories(block_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(block_io
    PRIVATE
        # List of libraries to link:
)