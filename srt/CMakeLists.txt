if (ANDROID)
    set(SUPPORTED_ABIS armeabi-v7a)
    if (NOT ${ANDROID_ABI} IN_LIST SUPPORTED_ABIS)
        message(FATAL_ERROR "target android abi ${ANDROID_ABI} is not supported.")
    endif()
else()
    message(FATAL_ERROR "target platform is not supported.")
endif()

add_library(wdeps-srt SHARED IMPORTED GLOBAL)
add_library(srt::srt ALIAS wdeps-srt)
set_target_properties(wdeps-srt PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/libsrt.so
    IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/libsrt.so
)

add_library(wdeps-srt-static STATIC IMPORTED GLOBAL)
add_library(srt::srt-static ALIAS wdeps-srt-static)
set_target_properties(wdeps-srt-static PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/libsrt.a
    IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/libsrt.a
)
