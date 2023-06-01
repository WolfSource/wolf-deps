cmake_minimum_required(VERSION 3.8)

project(wolf-deps)

option(WDEPS_BUILD_TESTS "executable to test target, build, and basic runtime integirty" OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(msquic)

if (WDEPS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
