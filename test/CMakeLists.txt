cmake_minimum_required(VERSION 3.0.0)

add_subdirectory(googletest)
# I have no idea what this means, but it works lol
# https://stackoverflow.com/questions/12540970/how-to-make-gtest-build-mdd-instead-of-mtd-by-default-using-cmake
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(This ControlTests)

set(Sources 
    test.cpp
)

add_executable(${This} ${Sources})
target_link_libraries(${This} PUBLIC
    gtest_main
)

add_test(
    NAME ${This}
    COMMAND ${This}
)