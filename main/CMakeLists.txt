cmake_minimum_required(VERSION 4.0)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(IM_CONTROL_SOURCES
    main.cpp
    argparse.cpp
    ../common/src/log.cpp
    ../im_control.manifest
)

add_executable(im-control ${IM_CONTROL_SOURCES})
target_include_directories(im-control PRIVATE ../common/include)

add_executable(im-controlw WIN32 ${IM_CONTROL_SOURCES})
target_compile_definitions(im-controlw PRIVATE IM_CONTROL_GUI)
target_include_directories(im-controlw PRIVATE ../common/include)

install(
    TARGETS im-control im-controlw
    RUNTIME DESTINATION .
)
