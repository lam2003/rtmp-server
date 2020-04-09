add_library(common
    reload.cpp
    config.cpp
    utils.cpp
    io.cpp
    socket.cpp
    st.cpp
    reader.cpp
    file.cpp
    thread.cpp
    log.cpp
    error.cpp
    buffer.cpp
    listener.cpp
    kbps.cpp
    connection.cpp
    sample.cpp
)

add_dependencies(common
    st
)

target_link_libraries(common
    libst.a
)
