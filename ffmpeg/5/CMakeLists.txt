if (NOT WIN32 AND NOT ANDROID)
    message(FATAL_ERROR "target platform is not supported.")
endif()

if (ANDROID)
    set(SUPPORTED_ABIS armeabi-v7a)
    if (NOT ${ANDROID_ABI} IN_LIST SUPPORTED_ABIS)
        message(FATAL_ERROR "target android abi ${ANDROID_ABI} is not supported.")
    endif()
endif()

include(GNUInstallDirs)

add_library(wdeps-ffmpeg INTERFACE)
add_library(ffmpeg::ffmpeg ALIAS wdeps-ffmpeg)

add_library(wdeps-ffmpeg-options INTERFACE)
target_include_directories(wdeps-ffmpeg-options INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

macro(add_ffmpeg_library LIBRARY_NAME LIBRARY_TYPE)
    add_library(wdeps-ffmpeg-${LIBRARY_NAME} ${LIBRARY_TYPE} IMPORTED GLOBAL)
    target_link_libraries(wdeps-ffmpeg-${LIBRARY_NAME} INTERFACE wdeps-ffmpeg-options)
    target_link_libraries(wdeps-ffmpeg INTERFACE wdeps-ffmpeg-${LIBRARY_NAME})
    if (WDEPS_INSTALL AND NOT ${LIBRARY_TYPE} STREQUAL "STATIC")
        install(
            IMPORTED_RUNTIME_ARTIFACTS wdeps-ffmpeg-${LIBRARY_NAME}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
endmacro()

if (WIN32)
    macro(add_ffmpeg_library_windows LIBRARY_NAME SHARED_FILEPATH STATIC_FILEPATH)
        add_ffmpeg_library(${LIBRARY_NAME} SHARED)
        set_target_properties(wdeps-ffmpeg-${LIBRARY_NAME} PROPERTIES
            IMPORTED_LOCATION ${SHARED_FILEPATH}
            IMPORTED_LOCATION_RELEASE ${SHARED_FILEPATH}
            IMPORTED_IMPLIB ${STATIC_FILEPATH}
        )
    endmacro()

    add_ffmpeg_library_windows(avcodec    ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/avcodec-59.dll   ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/avcodec.lib)
    add_ffmpeg_library_windows(avdevice   ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/avdevice-59.dll  ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/avdevice.lib)
    add_ffmpeg_library_windows(avfilter   ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/avfilter-8.dll   ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/avfilter.lib)
    add_ffmpeg_library_windows(avformat   ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/avformat-59.dll  ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/avformat.lib)
    add_ffmpeg_library_windows(avutil     ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/avutil-57.dll    ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/avutil.lib)
    add_ffmpeg_library_windows(postproc   ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/postproc-56.dll  ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/postproc.lib)
    add_ffmpeg_library_windows(swresample ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/swresample-4.dll ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/swresample.lib)
    add_ffmpeg_library_windows(swscale    ${CMAKE_CURRENT_SOURCE_DIR}/bin/win/swscale-6.dll    ${CMAKE_CURRENT_SOURCE_DIR}/lib/win/swscale.lib)
elseif(ANDROID)
    add_subdirectory(${PROJECT_SOURCE_DIR}/srt ${CMAKE_CURRENT_BINARY_DIR}/deps-srt)

    macro(add_ffmpeg_library_android LIBRARY_NAME)
        add_ffmpeg_library(${LIBRARY_NAME} SHARED)
        set_target_properties(wdeps-ffmpeg-${LIBRARY_NAME} PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.so
            IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.so
        )

        add_ffmpeg_library(${LIBRARY_NAME}-static STATIC)
        set_target_properties(wdeps-ffmpeg-${LIBRARY_NAME}-static PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.a
            IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.a
        )
    endmacro()

    add_ffmpeg_library_android(avcodec)
    add_ffmpeg_library_android(avdevice)
    add_ffmpeg_library_android(avfilter)
    add_ffmpeg_library_android(avformat)
    add_ffmpeg_library_android(avutil)
    add_ffmpeg_library_android(postproc)
    add_ffmpeg_library_android(swresample)
    add_ffmpeg_library_android(swscale)

    target_link_libraries(wdeps-ffmpeg INTERFACE srt::srt)
endif()

if (WDEPS_INSTALL)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION .)
endif()
