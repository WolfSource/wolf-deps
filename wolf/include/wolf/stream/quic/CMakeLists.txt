if (EMSCRIPTEN)
    message(FATAL_ERROR "WOLF_STREAM_QUIC is not supported for wasm target.")
endif()

FetchContent_Declare(
    msquic
    GIT_REPOSITORY https://github.com/microsoft/msquic
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(msquic)

file(GLOB_RECURSE WOLF_STREAM_QUIC_SOURCES *.hpp *.cpp)
target_sources(${PROJECT_NAME}
    PRIVATE
    ${WOLF_STREAM_QUIC_SOURCES}
)
target_link_libraries(${PROJECT_NAME} PUBLIC msquic)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    WOLF_STREAM_QUIC
)