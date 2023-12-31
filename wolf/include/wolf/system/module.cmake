set(SYSTEM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wolf/system/)

file(GLOB_RECURSE SYSTEM_SRCS
    ${SYSTEM_PATH}/w_gametime.cpp
    ${SYSTEM_PATH}/w_gametime.hpp
    ${SYSTEM_PATH}/w_trace.cpp
    ${SYSTEM_PATH}/w_trace.hpp
)

if (NOT EMSCRIPTEN)
    list(APPEND SYSTEM_SRCS
        ${SYSTEM_PATH}/w_leak_detector.cpp
        ${SYSTEM_PATH}/w_leak_detector.hpp
        ${SYSTEM_PATH}/w_process.cpp
        ${SYSTEM_PATH}/w_process.hpp
        ${SYSTEM_PATH}/w_time.cpp
        ${SYSTEM_PATH}/w_time.hpp
    )
endif()

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SYSTEM_SRCS}
)

source_group("system" FILES ${SYSTEM_SRCS})

if (WOLF_SYSTEM_MIMALLOC)
    vcpkg_install(mimalloc)
    find_package(mimalloc CONFIG REQUIRED)
    if (BUILD_SHARED_LIBS)
        target_link_libraries(${PROJECT_NAME} PRIVATE mimalloc)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE mimalloc-static)
    endif()
endif()

if (WOLF_SYSTEM_OPENSSL)
    # no source codes, only link against openssl library.
    vcpkg_install(openssl)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if (WOLF_SYSTEM_POSTGRESQL OR WOLF_SYSTEM_REDIS)
    include(${SYSTEM_PATH}/db/module.cmake)
endif()

if (WOLF_SYSTEM_GAMEPAD_CLIENT OR WOLF_SYSTEM_GAMEPAD_VIRTUAL)
    add_subdirectory(${SYSTEM_PATH}/gamepad)
endif()

if (WOLF_SYSTEM_LZ4 OR WOLF_SYSTEM_LZMA)
    include(${SYSTEM_PATH}/compression/module.cmake)
endif()

# if (WOLF_SYSTEM_LOG)
    include(${SYSTEM_PATH}log/module.cmake)
# endif()

# if (WOLF_SYSTEM_SOCKET OR WOLF_SYSTEM_HTTP_WS)
    add_subdirectory(${SYSTEM_PATH}/socket)
# endif()

if (WOLF_SYSTEM_LUA OR WOLF_SYSTEM_PYTHON)
    add_subdirectory(${SYSTEM_PATH}/script)
endif()
