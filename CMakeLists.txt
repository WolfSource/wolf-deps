cmake_minimum_required(VERSION 3.21)

if (NOT DEFINED WDPES_MAIN_PROJECT)
    set(WDEPS_MAIN_PROJECT OFF)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(WDEPS_MAIN_PROJECT ON)
    endif()
endif()

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

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

option(WDEPS_BUILD_TESTS "executable to test target, build, and basic runtime integirty" ${WDEPS_MAIN_PROJECT})
option(WDEPS_INSTALL "enable installation" ${WDEPS_MAIN_PROJECT})

feature_option(WDEPS_MSQUIC msquic "enable msquic library" WDEPS_MAIN_PROJECT AND ANDROID OR WIN32)
feature_option(WDEPS_FFMPEG5 ffmpeg/5 "enable ffmpeg-5 library" WDEPS_MAIN_PROJECT AND ANDROID_ABI STREQUAL "armeabi-v7a" OR WIN32)

if (WDEPS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
