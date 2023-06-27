if (ANDROID)
    set(SUPPORTED_ABIS armeabi-v7a)
    if (NOT ${ANDROID_ABI} IN_LIST SUPPORTED_ABIS)
        message(FATAL_ERROR "target android abi ${ANDROID_ABI} is not supported.")
    endif()
else()
    message(FATAL_ERROR "target platform is not supported.")
endif()

add_library(wdeps-svtav1 INTERFACE)
add_library(svtav1::svtav1 ALIAS wdeps-svtav1)

macro(add_svtav1_library TARGET_NAME LIBRARY_TYPE)
    add_library(wdeps-svtav1-${TARGET_NAME} ${LIBRARY_TYPE} IMPORTED GLOBAL)
    add_library(svtav1::${TARGET_NAME} ALIAS wdeps-svtav1-${TARGET_NAME})
    target_link_libraries(wdeps-svtav1 INTERFACE wdeps-svtav1-${TARGET_NAME})
endmacro()

if(ANDROID)
    macro(add_svtav1_library_android TARGET_NAME LIBRARY_NAME)
        add_svtav1_library(${TARGET_NAME} SHARED)
        set_target_properties(wdeps-svtav1-${TARGET_NAME} PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.so
            IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.so
        )

        add_svtav1_library(${TARGET_NAME}-static STATIC)
        set_target_properties(wdeps-svtav1-${TARGET_NAME}-static PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.a
            IMPORTED_LOCATION_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/android-${ANDROID_ABI}/lib${LIBRARY_NAME}.a
        )
    endmacro()

    add_svtav1_library_android(encoder SvtAv1Enc)
    add_svtav1_library_android(decoder SvtAv1Dec)
endif()
