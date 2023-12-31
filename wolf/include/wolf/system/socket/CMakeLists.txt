# if (WOLF_SYSTEM_SOCKET)
    set(WOLF_SYSTEM_SOCKET_HEADERS
        w_socket_options.hpp
        w_tcp_client.hpp
        w_tcp_server.hpp
    )
    set(WOLF_SYSTEM_SOCKET_SOURCES
        w_tcp_client.cpp
        w_tcp_server.cpp
    )
    target_sources(${PROJECT_NAME}
        PRIVATE
            ${WOLF_SYSTEM_SOCKET_HEADERS}
            ${WOLF_SYSTEM_SOCKET_SOURCES}
    )

    find_package(Boost ${BOOST_VERSION} REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)
	target_compile_definitions(${PROJECT_NAME} PUBLIC WOLF_SYSTEM_SOCKET)
# endif()

if (WOLF_SYSTEM_HTTP_WS)
    if (EMSCRIPTEN)
        set(WOLF_SYSTEM_HTTP_WS_HEADERS w_ws_client_emc.hpp)
        set(WOLF_SYSTEM_HTTP_WS_SOURCES w_ws_client_emc.cpp)
    else()
        set(WOLF_SYSTEM_HTTP_WS_HEADERS
            w_ws_client.hpp
            w_ws_server.hpp
        )
        set(WOLF_SYSTEM_HTTP_WS_SOURCES
            w_ws_client.cpp
            w_ws_server.cpp
        )
    endif()
    target_sources(${PROJECT_NAME}
        PRIVATE
            ${WOLF_SYSTEM_HTTP_WS_HEADERS}
            ${WOLF_SYSTEM_HTTP_WS_SOURCES}
    )

    find_package(Boost ${BOOST_VERSION} REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)
endif()
