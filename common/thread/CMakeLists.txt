add_library(thread
    thread.cpp
)

add_dependencies(thread
    st
)

target_link_libraries(thread
    libst.a
)