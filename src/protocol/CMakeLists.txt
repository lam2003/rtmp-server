add_library(protocol
    amf/amf0.cpp
    rtmp/stack.cpp
    rtmp/source.cpp
    rtmp/edge.cpp
    rtmp/packet.cpp
    rtmp/message.cpp
    rtmp/handshake.cpp
    rtmp/jitter.cpp
    rtmp/connection.cpp
    rtmp/recv_thread.cpp
    rtmp/dvr.cpp
    rtmp/consumer.cpp
    rtmp/gop_cache.cpp
    rtmp/server.cpp
)

add_dependencies(protocol
    common
)

target_link_libraries(protocol
    common
)