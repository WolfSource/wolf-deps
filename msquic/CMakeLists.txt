# set up PLATFORM to windows,android-arm64-v8a,etc directories.
# set up LIBRARY_FILENAME to the appropriate filename.
if (WIN32)
    set(PLATFORM windows)
    set(LIBRARY_FILENAME msquic.dll)
elseif(ANDROID)
    set(SUPPORTED_ABIS arm64-v8a armeabi-v7a x86_64)
    if (NOT ${ANDROID_ABI} IN_LIST SUPPORTED_ABIS)
        message(FATAL_ERROR "target android abi ${ANDROID_ABI} is not supported.")
    endif()

    set(PLATFORM android-${ANDROID_ABI})
    set(LIBRARY_FILENAME libmsquic.so)
else()
    message(FATAL_ERROR "target platform is not supported.")
endif()

add_library(wdeps-msquic SHARED IMPORTED GLOBAL)
add_library(msquic::msquic ALIAS wdeps-msquic)

target_include_directories(wdeps-msquic INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(wdeps-msquic PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM}/Release/${LIBRARY_FILENAME}
    IMPORTED_LOCATION_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM}/Debug/${LIBRARY_FILENAME}
    IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM}/Release/${LIBRARY_FILENAME}
    IMPORTED_CONFIGURATIONS "Release;Debug"
)

if (WIN32)
    set_target_properties(wdeps-msquic PROPERTIES
        IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/Release/msquic.lib
        IMPORTED_IMPLIB_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/Debug/msquic.lib
        IMPORTED_IMPLIB_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/Release/msquic.lib
    )
    target_link_libraries(wdeps-msquic INTERFACE ws2_32 schannel ntdll)
endif()
