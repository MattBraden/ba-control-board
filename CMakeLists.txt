cmake_minimum_required(VERSION 3.0.0)

set(This control-board)

project(${This} VERSION 0.1.0)

enable_testing()

add_library(
    LED SHARED
    lib/LED/LED.cpp
    lib/LED/LED.h
)

add_library(
    JSON SHARED
    lib/JSON/JSON.cpp
    lib/JSON/JSON.h
)

add_subdirectory(test)