
add_library(io
    io.cpp
    socket.cpp
    st.cpp
    reader.cpp
    file_reader.cpp
)

add_dependencies(io
    st
)

target_link_libraries(io
    libst.a
)