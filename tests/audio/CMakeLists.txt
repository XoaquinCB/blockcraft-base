add_executable(audio_test)

target_sources(audio_test
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/audio_test.c
)

target_link_libraries(audio_test
    PRIVATE
        # List of libraries to link:
        audio
)

pico_add_extra_outputs(audio_test)
