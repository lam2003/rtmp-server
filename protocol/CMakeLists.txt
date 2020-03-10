add_library(protocol
    rtmp/stack.cpp
    amf/amf0.cpp
    rtmp/source.cpp
    rtmp/edge.cpp
    rtmp/packet.cpp
    rtmp/message.cpp
    rtmp/handshake.cpp
    rtmp/jitter.cpp
)

add_dependencies(protocol
    common
)

target_link_libraries(protocol
    common
)