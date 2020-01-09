add_library(protocol
    rtmp_stack.cpp
    rtmp_amf0.cpp
)

add_dependencies(protocol
    common
)

target_link_libraries(protocol
    common
)