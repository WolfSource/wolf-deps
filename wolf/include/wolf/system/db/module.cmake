if (MOBILE)
    message(FATAL_ERROR "WOLF_SYSTEM_POSTGRESQL and WOLF_SYSTEM_REDIS are not supported for mobile target")
elseif(EMSCRIPTEN)
    message(FATAL_ERROR "WOLF_SYSTEM_POSTGRESQL and WOLF_SYSTEM_REDIS are not supported for the wasm32 target on Emscripten")
endif()

set(DB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wolf/system/db)

file(GLOB_RECURSE DB_SRCS)

if (WOLF_SYSTEM_POSTGRESQL)
    list(APPEND DB_SRCS
        ${DB_PATH}/w_postgresql.cpp
        ${DB_PATH}/w_postgresql.hpp
    )

    vcpkg_install(libpq)
    find_package(PostgreSQL REQUIRED)
    target_sources(${PROJECT_NAME} PRIVATE ${DB_SRCS})

    target_link_libraries(${PROJECT_NAME} PUBLIC PostgreSQL::PostgreSQL)
endif()

if (WOLF_SYSTEM_REDIS)
    if (NOT WOLF_SYSTEM_OPENSSL)
        message(FATAL_ERROR "WOLF_SYSTEM_REDIS requires WOLF_SYSTEM_OPENSSL")
    endif()

    list(APPEND DB_SRCS
        ${DB_PATH}/w_redis_client.cpp
        ${DB_PATH}/w_redis_client.hpp
    )

    vcpkg_install(boost-json)
    find_package(Boost REQUIRED COMPONENTS json)

    message("fetching https://github.com/boostorg/redis.git")
    FetchContent_Declare(
        boost_redis
        GIT_REPOSITORY https://github.com/boostorg/redis.git
        GIT_TAG        develop
        GIT_SHALLOW    ON
    )
    set(BOOST_REDIS_INSTALL OFF)
    set(BOOST_REDIS_TESTS OFF)
    set(BOOST_REDIS_EXAMPLES OFF)
    set(BOOST_REDIS_BENCHMARKS OFF)
    set(BOOST_REDIS_DOC OFF)
    FetchContent_MakeAvailable(boost_redis)

    target_include_directories(${PROJECT_NAME} PUBLIC ${boost_redis_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::json)
endif()

target_sources(${PROJECT_NAME}
    PRIVATE
    ${DB_SRCS}
)

source_group("system/db" FILES ${DB_SRCS})


