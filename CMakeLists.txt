# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wno-unused-parameter)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(debugger C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

file(GLOB_RECURSE sources
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(debugger ${sources} )

pico_set_program_name(debugger "debugger")
pico_set_program_version(debugger "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(debugger 0)
pico_enable_stdio_usb(debugger 1)

# Add the standard library to the build
target_link_libraries(debugger
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(debugger PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

#pico_enable_stdio_usb(debugger 1)
pico_add_extra_outputs(debugger)


target_include_directories(debugger PUBLIC ${CMAKE_SOURCE_DIR}/inc)
