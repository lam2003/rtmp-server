add_library(codec
    codec.cpp
    avc.cpp
    aac.cpp
)

add_dependencies(codec
    common
)

target_link_libraries(codec
    common
)