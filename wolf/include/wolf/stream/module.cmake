set(STREAM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wolf/stream/)

if (WOLF_STREAM_GRPC)
    include(${STREAM_PATH}/rpc/module.cmake)
endif()

if (WOLF_STREAM_QUIC)
    include(${STREAM_PATH}/quic/module.cmake)
endif()

if (WOLF_STREAM_JANUS)
    include(${STREAM_PATH}/janus/module.cmake)
endif()

if (WOLF_STREAM_RIST)
    include(${STREAM_PATH}/rist/module.cmake)
endif()

if (WOLF_STREAM_WEBRTC)
    include(${STREAM_PATH}/webrtc/module.cmake)
endif()
