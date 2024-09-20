cmake_minimum_required(VERSION 3.5)
project(shipwright)

file(GLOB_RECURSE sources src/*.cpp src/*.hpp)

add_executable(shipwright ${sources})

target_compile_options(shipwright PUBLIC -g)

target_include_directories(shipwright PUBLIC src)


