add_library(protocol
    rtmp_stack.cpp
)

add_dependencies(protocol
    common
)

target_link_libraries(protocol
    common
)