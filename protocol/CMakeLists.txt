add_library(protocol
    rtmp_stack.cpp
    amf0.cpp
    rtmp_source.cpp
    rtmp_edge.cpp
    rtmp_packet.cpp
    rtmp_message.cpp
    rtmp_handshake.cpp
)

add_dependencies(protocol
    common
)

target_link_libraries(protocol
    common
)