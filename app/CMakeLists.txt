add_executable(rtmp_server EXCLUDE_FROM_ALL
    main.cpp
    server.cpp
    rtmp_recv_thread.cpp
    rtmp_connection.cpp
    rtmp_server.cpp
    dvr.cpp
)

add_dependencies(rtmp_server
   common
   protocol
)

target_link_libraries(rtmp_server
    common
    protocol
)
