if (ANDROID)
    set(SUPPORTED_ABIS armeabi-v7a)
    if (NOT ${ANDROID_ABI} IN_LIST SUPPORTED_ABIS)
        message(FATAL_ERROR "target android abi ${ANDROID_ABI} is not supported.")
    endif()
else()
    message(FATAL_ERROR "target platform is not supported.")
endif()

if (ANDROID)
    add_library(wdeps-dav1d-static STATIC IMPORTED GLOBAL)
    add_library(dav1d::dav1d-static ALIAS wdeps-dav1d-static)
    set_target_properties(wdeps-dav1d-static PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/libdav1d.a
        IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/libdav1d.a
    )
endif()
