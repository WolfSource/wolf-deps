if (WOLF_SYSTEM_GAMEPAD_CLIENT)
    if (EMSCRIPTEN)
        message(FATAL_ERROR "WOLF_SYSTEM_GAMEPAD_CLIENT feature is not supported for wasm/emscripten target.")
    endif()

    set(WOLF_SYSTEM_GAMEPAD_CLIENT_HEADERS
        w_gamepad_client_keymap.hpp
        w_gamepad_client_types.hpp
        w_gamepad_client.hpp
    )
    set(WOLF_SYSTEM_GAMEPAD_CLIENT_SOURCES
        w_gamepad_client_emc.cpp
        w_gamepad_client_sdl.cpp
    )
    target_sources(${PROJECT_NAME}
    PRIVATE
        ${WOLF_SYSTEM_GAMEPAD_CLIENT_HEADERS}
        ${WOLF_SYSTEM_GAMEPAD_CLIENT_SOURCES}
    )

    message("fetching https://github.com/libsdl-org/SDL")
        FetchContent_Declare(
        SDL3-static
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG        main
        GIT_SHALLOW    ON
    )

    set(SDL_AUDIO OFF)
    set(SDL_DIRECTX OFF)
    set(SDL_DISKAUDIO OFF)
    set(SDL_DUMMYAUDIO OFF)
    set(SDL_DUMMYVIDEO OFF)
    set(SDL_FILE OFF)
    set(SDL_FILESYSTEM OFF)
    set(SDL_METAL OFF)
    set(SDL_OFFSCREEN OFF)
    set(SDL_OPENGL OFF)
    set(SDL_OPENGLES OFF)
    set(SDL_RENDER OFF)
    set(SDL_RENDER_D3D OFF)
    set(SDL_RENDER_METAL OFF)
    set(SDL_SHARED OFF)
    set(SDL_TEST OFF)
    set(SDL_TESTS OFF)
    set(SDL_VIDEO OFF)
    set(SDL_VULKAN OFF)
    set(SDL_WASAPI OFF)
    set(SDL_HIGHDAPI_JOYSTICK ON)
    set(SDL_JOYSTICK ON)
    set(SDL_STATIC ON)
    set(SDL_XINPUT ON)
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(SDL3-static)

    target_link_libraries(${PROJECT_NAME} PUBLIC SDL3-static)

    set_target_properties(
        SDL3-static
        uninstall
        PROPERTIES FOLDER "SDL"
    )
endif()

if (WOLF_SYSTEM_GAMEPAD_VIRTUAL)
    if (NOT WIN64)
        message(FATAL_ERROR "WOLF_SYSTEM_GAMEPAD_VIRTUAL is only supported for windows.")
    endif()

    message("fetching https://github.com/ViGEm/ViGEmClient.git")
    FetchContent_Declare(
        ViGEmClient
        GIT_REPOSITORY https://github.com/ViGEm/ViGEmClient.git
        GIT_TAG        master
        GIT_SHALLOW    ON
    )
    FetchContent_MakeAvailable(ViGEmClient)

    set(WOLF_SYSTEM_GAMEPAD_VIRTUAL_HEADERS
        w_gamepad_virtual_pool.hpp
        w_gamepad_virtual.hpp
    )
    set(WOLF_SYSTEM_GAMEPAD_VIRTUAL_SOURCES
        w_gamepad_virtual_pool.cpp
        w_gamepad_virtual.cpp
    )
    target_sources(${PROJECT_NAME}
        PRIVATE
            ${WOLF_SYSTEM_GAMEPAD_VIRTUAL_HEADERS}
            ${WOLF_SYSTEM_GAMEPAD_VIRTUAL_SOURCES}
    )

    # target_include_directories(${PROJECT_NAME} PUBLIC ${ViGEmClient_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PUBLIC ViGEmClient)
    target_link_libraries(${PROJECT_NAME} PRIVATE Xinput.lib SetupAPI.lib)
endif()
