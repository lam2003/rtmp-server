add_executable(rtmp_server EXCLUDE_FROM_ALL
    main.cpp
)

add_dependencies(rtmp_server
    thread
    io
    log
    config
)

target_link_libraries(rtmp_server
    thread
    io
    log
    config
)
