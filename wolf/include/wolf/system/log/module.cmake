if (EMSCRIPTEN)
    message(FATAL_ERROR "WOLF_SYSTEM_LOG is not supported for wasm target.")
endif()

set(LOG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wolf/system/log)

file(GLOB_RECURSE LOG_SRCS)

list(APPEND LOG_SRCS
    ${LOG_PATH}/w_log.cpp
    ${LOG_PATH}/w_log.hpp
    ${LOG_PATH}/w_log_config.hpp
)
vcpkg_install(spdlog)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog spdlog::spdlog_header_only)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${LOG_SRCS}
)

source_group("system/log" FILES ${COMPRESSION_SRCS})
