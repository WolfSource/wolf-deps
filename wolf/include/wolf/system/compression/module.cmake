set(COMPRESSION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wolf/system/compression)

file(GLOB_RECURSE COMPRESSION_SRCS)

if (WOLF_SYSTEM_LZ4)
    if (EMSCRIPTEN)
        message(FATAL_ERROR "the wasm32 target is not supported for WOLF_SYSTEM_LZ4")
    endif()

    list(APPEND COMPRESSION_SRCS
        ${COMPRESSION_PATH}/w_lz4.cpp
        ${COMPRESSION_PATH}/w_lz4.hpp
    )

    vcpkg_install(lz4)
    find_package(lz4 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC lz4::lz4)
endif()

if (WOLF_SYSTEM_LZMA)
    if (EMSCRIPTEN)
          message(FATAL_ERROR "the wasm32 target is not supported for WOLF_SYSTEM_LZMA")
    endif()

    message("fetching https://github.com/WolfSource/lzma.git")
    FetchContent_Declare(
        lzma
        GIT_REPOSITORY https://github.com/WolfSource/lzma.git
        GIT_TAG        main
        GIT_SHALLOW    ON
    )
    FetchContent_MakeAvailable(lzma)

    list(APPEND COMPRESSION_SRCS
        ${COMPRESSION_PATH}/w_lzma.cpp
        ${COMPRESSION_PATH}/w_lzma.hpp
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${lzma_SOURCE_DIR}/src)
    target_link_libraries(${PROJECT_NAME} PUBLIC lzma)
endif()

target_sources(${PROJECT_NAME}
    PRIVATE
    ${COMPRESSION_SRCS}
)

source_group("system/compression" FILES ${COMPRESSION_SRCS})
