add_executable(rtmp_server EXCLUDE_FROM_ALL
    main.cpp
    server.cpp
    listener.cpp
)

add_dependencies(rtmp_server
   common
)

target_link_libraries(rtmp_server
    common
)
