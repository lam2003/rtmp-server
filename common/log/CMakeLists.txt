add_library(log
    log.cpp
    fast_log.cpp
)

add_dependencies(log
    st
    config
)
