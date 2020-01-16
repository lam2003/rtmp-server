add_executable(rtmp_server EXCLUDE_FROM_ALL
    main.cpp
    server.cpp
    recv_thread.cpp
    rtmp_connection.cpp
    rtmp_server.cpp
)

add_dependencies(rtmp_server
   common
   protocol
)

target_link_libraries(rtmp_server
    common
    protocol
)
