if (ANDROID)
    set(SUPPORTED_ABIS armeabi-v7a)
    if (NOT ${ANDROID_ABI} IN_LIST SUPPORTED_ABIS)
        message(FATAL_ERROR "target android abi ${ANDROID_ABI} is not supported.")
    endif()
else()
    message(FATAL_ERROR "target platform is not supported.")
endif()

if (ANDROID)
    add_library(wdeps-x264 SHARED IMPORTED GLOBAL)
    add_library(x264::x264 ALIAS wdeps-x264)
    set_target_properties(wdeps-x264 PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/libx264.so.164
        IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/libx264.so.164
    )

    add_library(wdeps-x264-static STATIC IMPORTED GLOBAL)
    add_library(x264::x264-static ALIAS wdeps-x264-static)
    set_target_properties(wdeps-x264-static PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/libx264.a
        IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/libx264.a
    )
endif()
