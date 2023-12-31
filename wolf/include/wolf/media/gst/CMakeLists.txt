file(GLOB_RECURSE WOLF_MEDIA_GSTREAMER_HEADERS *.hpp)
file(GLOB_RECURSE WOLF_MEDIA_GSTREAMER_SOURCES *.cpp)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${WOLF_MEDIA_GSTREAMER_HEADERS}
        ${WOLF_MEDIA_GSTREAMER_SOURCES}
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(gstreamer REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-video-1.0
    gstreamer-audio-1.0
)

add_library(gstreamer-lib INTERFACE)
target_compile_options(gstreamer-lib INTERFACE ${gstreamer_CFLAGS})
target_include_directories(gstreamer-lib INTERFACE ${gstreamer_INCLUDE_DIRS})
target_link_directories(gstreamer-lib BEFORE INTERFACE ${gstreamer_LIBRARY_DIRS})
target_link_libraries(gstreamer-lib INTERFACE ${gstreamer_LIBRARIES})

target_link_libraries(${PROJECT_NAME} PUBLIC gstreamer-lib)
target_compile_definitions(${PROJECT_NAME} PUBLIC WOLF_MEDIA_GSTREAMER)
