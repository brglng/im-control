cmake_minimum_required(VERSION 4.0)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

add_executable(im-control WIN32
    main.cpp
    argparse.cpp
    ../common/src/log.cpp
)

target_include_directories(im-control PRIVATE ../common/include)

install(
    TARGETS im-control
    RUNTIME DESTINATION .
)
