if (WOLF_MEDIA_STB)
    add_subdirectory(stb)
endif()

if (WOLF_MEDIA_FFMPEG)
    add_subdirectory(ffmpeg)
endif()

if (WOLF_MEDIA_GSTREAMER)
    add_subdirectory(gst)
endif()

# if (WOLF_MEDIA_OPENAL)
    add_subdirectory(openal)
# endif()

if (WOLF_MEDIA_SCREEN_CAPTURE)
    message(FATAL_ERROR "WOLF_MEDIA_SCREEN_CAPTURE is not implemented yet.")
endif()
