cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME ScoreBoard)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
# Scoreboard application (ported 2016 firmware), one folder per peripheral/function
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/App/*.c")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${APP_SOURCES}
)


# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    App/Main
    App/Console
    App/Services
    App/Usart
    App/Timers
    App/Rtc
    App/Rng
    App/Eeprom
    App/Oled
    App/Matrix
    App/Gpio
    App/Usb
    App/Game
    App/Modes
    App/Ble
    App/Flash
)

# Add project symbols (macros)
# LED panel rows driven: 64 = E line (HUB75 pin 8) wired to PC2, all 4 lines
# (the board since the MCU replacement); 32 = E tied to GND (old board,
# lines 1 and 3 only). Change with: cmake --preset Debug -DMATRIX_PANEL_ROWS=32
# The display drawing code runs for every frame: optimise it in Debug builds
# too (at -O0 a full matrix rebuild takes ~30 ms and slows the App loop).
set_source_files_properties(
    App/Matrix/matrix.c App/Matrix/font_tiny.c App/Modes/scenes.c
    App/Modes/ui_bands.c App/Modes/ui_text_line.c
    PROPERTIES COMPILE_OPTIONS "-O2")

set(MATRIX_PANEL_ROWS 64 CACHE STRING "LED panel rows driven (32 or 64)")
set_property(CACHE MATRIX_PANEL_ROWS PROPERTY STRINGS 32 64)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    MATRIX_PANEL_ROWS=${MATRIX_PANEL_ROWS}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
