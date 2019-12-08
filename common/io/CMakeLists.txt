
add_library(io
    io.cpp
    socket.cpp
    st.cpp
)

add_dependencies(io
    st
)

target_link_libraries(io
    libst.a
)