if (WOLF_SYSTEM_LUA)
    set(WOLF_SYSTEM_LUA_HEADERS w_lua.hpp)
    set(WOLF_SYSTEM_LUA_SOURCES w_lua.cpp)
    target_sources(${PROJECT_NAME}
        PRIVATE
            ${WOLF_SYSTEM_LUA_HEADERS}
            ${WOLF_SYSTEM_LUA_SOURCES}
    )

    vcpkg_install(lua sol2)
    find_package(lua REQUIRED)
    find_package(sol2 CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC lua sol2)
endif()

if (WOLF_SYSTEM_PYTHON)
    vcpkg_install(boost-python)
    find_package(Python3 REQUIRED COMPONENTS Development)
    find_package(Boost REQUIRED COMPONENTS python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})

    set(WOLF_SYSTEM_PYTHON_HEADERS w_python.hpp)
    set(WOLF_SYSTEM_PYTHON_SOURCES w_python.cpp)
    target_sources(${PROJECT_NAME}
        PRIVATE
            ${WOLF_SYSTEM_PYTHON_HEADERS}
            ${WOLF_SYSTEM_PYTHON_SOURCES}
    )

    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            Python3::Python
            Boost::boost
            Boost::python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}
    )

    get_filename_component(PYTHON_HOME ${Python3_EXECUTABLE} DIRECTORY)
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        BOOST_PYTHON_STATIC_LIB
        PYTHON_HOME="${PYTHON_HOME}"
    )
endif()
