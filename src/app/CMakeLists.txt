add_executable(rtmp_server EXCLUDE_FROM_ALL
    main.cpp
    server.cpp
)

add_dependencies(rtmp_server
   common
   protocol
   codec
   muxer
)

target_link_libraries(rtmp_server
    common
    protocol
    codec
    muxer
    tcmalloc
    profiler
)
