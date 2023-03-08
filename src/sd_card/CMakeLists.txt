add_library(sd_card)

target_sources(sd_card
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/sd_card.c
    PUBLIC
        # List of public header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/sd_card.h
)

target_include_directories(sd_card
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(sd_card
    PRIVATE
        # List of libraries to link:
        FatFs_SPI
)