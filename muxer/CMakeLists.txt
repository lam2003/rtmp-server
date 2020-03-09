add_library(muxer
    flv.cpp
    muxer.cpp
)

add_dependencies(muxer
    common
    codec
)

target_link_libraries(muxer
    common
    codec
)