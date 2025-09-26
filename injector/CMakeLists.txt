cmake_minimum_required(VERSION 4.0)

project(im-select-injector LANGUAGES CXX)

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

add_library(im-select-hook-${bitness} MODULE
    hook.cpp
)

target_include_directories(im-select-hook-${bitness} PRIVATE ../common/include)

add_executable(im-select-injector-${bitness}
    injector.cpp
)

target_include_directories(im-select-injector-${bitness} PRIVATE ../common/include)
