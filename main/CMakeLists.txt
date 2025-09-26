cmake_minimum_required(VERSION 4.0)

add_executable(im-select
    main.cpp
)

target_include_directories(im-select PRIVATE ../common/include)
