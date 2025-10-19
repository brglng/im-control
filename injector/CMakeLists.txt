cmake_minimum_required(VERSION 4.0)

project(im-control-injector LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DWIN64)
    set(bitness "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-DWIN32)
    set(bitness "32")
else()
    message(FATAL_ERROR "Unknown platform")
endif()

add_library(im-control-hook-${bitness} MODULE
    hook.cpp
    ../common/src/log.cpp
)

target_include_directories(im-control-hook-${bitness} PRIVATE ../common/include)

add_executable(im-control-injector-${bitness} WIN32
    injector.cpp
    ../common/src/log.cpp
)

target_include_directories(im-control-injector-${bitness} PRIVATE ../common/include)

install(
    TARGETS im-control-hook-${bitness} im-control-injector-${bitness}
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
)
