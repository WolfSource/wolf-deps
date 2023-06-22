cmake_minimum_required(VERSION 3.8)

macro(feature_option FEATURE_NAME SUBDIRECTORY DESCRIPTION)
    if (${ARGN})
        set(DEFAULT_VALUE ON)
    else()
        set(DEFAULT_VALUE OFF)
    endif()
    option(${FEATURE_NAME} ${DESCRIPTION} ${DEFAULT_VALUE})
    if (${FEATURE_NAME})
        add_subdirectory(${SUBDIRECTORY})
    endif()
endmacro()

project(wolf-deps)

option(WDEPS_BUILD_TESTS "executable to test target, build, and basic runtime integirty" OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

feature_option(WDEPS_MSQUIC msquic "enable msquic library" ON)

if (WDEPS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
