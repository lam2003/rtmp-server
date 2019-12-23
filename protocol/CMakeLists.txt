add_library(protocol
    kbps.cpp
    rtmp_message.cpp
    rtmp_stack.cpp
)

add_dependencies(protocol
    common
)

target_link_libraries(protocol
    common
)