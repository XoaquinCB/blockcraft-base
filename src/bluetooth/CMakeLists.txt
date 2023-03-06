add_library(bluetooth)

target_sources(bluetooth
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/bluetooth.c
    PUBLIC
        # List of public header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/bluetooth.h
)

target_include_directories(bluetooth
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bluetooth
    PRIVATE
        # List of libraries to link:
)