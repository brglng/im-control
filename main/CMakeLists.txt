cmake_minimum_required(VERSION 4.0)

add_executable(im-control
    main.cpp
    ../common/src/log.cpp
)

target_include_directories(im-control PRIVATE ../common/include)

install(
    TARGETS im-control
    RUNTIME DESTINATION .
)
