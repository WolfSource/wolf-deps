set(WOLF_MEDIA_OPENAL_HEADERS w_openal.hpp)
set(WOLF_MEDIA_OPENAL_SOURCES w_openal.cpp)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${WOLF_MEDIA_OPENAL_HEADERS}
        ${WOLF_MEDIA_OPENAL_SOURCES}
)

message("fetching https://github.com/kcat/openal-soft.git")
FetchContent_Declare(
    openal
    GIT_REPOSITORY https://github.com/kcat/openal-soft.git
    GIT_TAG        master
    GIT_SHALLOW    ON
)
set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_INSTALL_EXAMPLES OFF)
set(LIBTYPE SHARED)
FetchContent_MakeAvailable(openal)

target_link_libraries(${PROJECT_NAME} PUBLIC OpenAL::OpenAL)
target_compile_definitions(${PROJECT_NAME} PUBLIC WOLF_MEDIA_OPENAL)

set_target_properties(
    build_version
    # common
    # ex-common
    OpenAL
    openal-info
    PROPERTIES FOLDER "OpenAL"
)
