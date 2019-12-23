add_executable(rtmp_server EXCLUDE_FROM_ALL
    main.cpp
    server.cpp
    listener.cpp
    connection.cpp
    recv_thread.cpp
    rtmp_connection.cpp
)

add_dependencies(rtmp_server
   common
   protocol
)

target_link_libraries(rtmp_server
    common
    protocol
)
