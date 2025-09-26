cmake_minimum_required(VERSION 4.0)

project(im-select-win LANGUAGES CXX)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/injector -B ${CMAKE_CURRENT_BINARY_DIR}/injector32 -G "${CMAKE_GENERATOR}" -A Win32
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/injector -B ${CMAKE_CURRENT_BINARY_DIR}/injector64 -G "${CMAKE_GENERATOR}" -A x64
        COMMAND_ERROR_IS_FATAL ANY
    )
else()
    message(FATAL_ERROR "Only Visual Studio generator is supported.")
endif()

add_custom_target(
    build_injector32
    ALL
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/injector32 --config ${CMAKE_BUILD_TYPE}
)
add_custom_target(
    build_injector64
    ALL
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/injector64 --config ${CMAKE_BUILD_TYPE}
)

add_subdirectory(main)
