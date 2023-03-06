add_executable(sd_card_test)

target_sources(sd_card_test
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/sd_card_test.c
)

target_link_libraries(sd_card_test
    PRIVATE
        # List of libraries to link:
        sd_card
)

pico_add_extra_outputs(sd_card_test)
