set(WOLF_MEDIA_FFMPEG_HEADERS
    w_av_config.hpp
    w_av_format.hpp
    w_av_frame.hpp
    w_av_packet.hpp
    w_decoder.hpp
    w_encoder.hpp
    w_ffmpeg_ctx.hpp
    w_ffmpeg.hpp
)
set(WOLF_MEDIA_FFMPEG_SOURCES
    w_av_config.cpp
    w_av_format.cpp
    w_av_frame.cpp
    w_av_packet.cpp
    w_decoder.cpp
    w_encoder.cpp
    w_ffmpeg_ctx.cpp
    w_ffmpeg.cpp
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${WOLF_MEDIA_FFMPEG_HEADERS}
        ${WOLF_MEDIA_FFMPEG_SOURCES}
)
message("PATH_FFMPEG ")
# set(PATH_FFMPEG 
# /mnt/c/github/wolfengine/wolf/build/android-arm-clang-shared/_deps/wolf-deps-src/ffmpeg/ffmpeg-6.0)

set(PATH_FFMPEG 
"C:/github/wolfengine/wolf/build/windows-x64-msvc-shared/_deps/wolf-deps-src/ffmpeg/6")

message("PATH_FFMPEG  ${PATH_FFMPEG}")

# dependency is availbale by wolf-deps in root cmake.
# target_link_libraries(${PROJECT_NAME} PUBLIC ffmpeg::ffmpeg)
# target_link_libraries(${PROJECT_NAME} PUBLIC 
				# ${PATH_FFMPEG}/lib/libavcodec.so
				# ${PATH_FFMPEG}/lib/libavdevice.so
				# ${PATH_FFMPEG}/lib/libavfilter.so
				# ${PATH_FFMPEG}/lib/libavformat.so
				# ${PATH_FFMPEG}/lib/libavutil.so
				# ${PATH_FFMPEG}/lib/libpostproc.so
				# ${PATH_FFMPEG}/lib/libswresample.so
				# ${PATH_FFMPEG}/lib/libswscale.so
				# )
				
				
target_link_libraries(${PROJECT_NAME} PUBLIC 
				${PATH_FFMPEG}/lib/win/avcodec.lib
				${PATH_FFMPEG}/lib/win/avdevice.lib
				${PATH_FFMPEG}/lib/win/avfilter.lib
				${PATH_FFMPEG}/lib/win/avformat.lib
				${PATH_FFMPEG}/lib/win/avutil.lib
				# ${PATH_FFMPEG}/lib/win/postproc.lib
				${PATH_FFMPEG}/lib/win/swresample.lib
				${PATH_FFMPEG}/lib/win/swscale.lib
				)
				
target_include_directories(${PROJECT_NAME} PUBLIC 
				${PATH_FFMPEG}/include
				)
target_compile_definitions(${PROJECT_NAME} PUBLIC WOLF_MEDIA_FFMPEG)
