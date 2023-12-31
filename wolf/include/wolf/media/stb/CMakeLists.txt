set(WOLF_MEDIA_STB_HEADERS w_image.hpp)
set(WOLF_MEDIA_STB_SOURCES w_image.cpp)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${WOLF_MEDIA_STB_HEADERS}
        ${WOLF_MEDIA_STB_SOURCES}
)

message("fetching https://github.com/nothings/stb.git")
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
  GIT_SHALLOW    ON
)
FetchContent_Populate(stb)

target_include_directories(${PROJECT_NAME} PUBLIC ${stb_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PUBLIC WOLF_MEDIA_STB)
