include(CTest)

macro(postbuild_dll_copy TARGET_NAME)
    if (WIN32)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET_NAME}> $<TARGET_FILE_DIR:${TARGET_NAME}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endmacro()

if (WDEPS_MSQUIC)
    add_executable(msquic-test msquic.cpp)
    target_link_libraries(msquic-test PRIVATE msquic::msquic)
    add_test(NAME msquic-test COMMAND msquic-test)
    postbuild_dll_copy(msquic-test)
endif()

if (WDEPS_FFMPEG5)
    add_executable(ffmpeg5-test ffmpeg5.cpp)
    target_link_libraries(ffmpeg5-test PRIVATE ffmpeg::ffmpeg)
    add_test(NAME ffmpeg5-test COMMAND ffmpeg5-test)
    postbuild_dll_copy(ffmpeg5-test)
endif()
