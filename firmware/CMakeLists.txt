cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

project(keyboard)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(keyboard
  src/keyboard.c
  src/mcp4921.c
  src/key_matrix.c
  include/mcp4921.h
  include/key_matrix.h
)

target_include_directories(keyboard PRIVATE include)

target_link_libraries(keyboard pico_stdlib hardware_spi)


pico_add_extra_outputs(keyboard)
