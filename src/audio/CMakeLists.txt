add_library(audio)

target_sources(audio
    PRIVATE
        # List of private source and header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/audio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hw_config.c
    PUBLIC
        # List of public header files:
        ${CMAKE_CURRENT_SOURCE_DIR}/audio.h
)

target_include_directories(audio
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio
    PUBLIC
        # List of libraries to link:
        FatFs_SPI
        hardware_gpio
        hardware_dma
        hardware_pwm
        hardware_irq
        hardware_sync
)