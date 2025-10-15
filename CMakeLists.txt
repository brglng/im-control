cmake_minimum_required(VERSION 4.0)

project(im-control LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH "Install path" FORCE)
endif()

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/injector -B ${CMAKE_CURRENT_BINARY_DIR}/injector32 -G "${CMAKE_GENERATOR}" -A Win32 -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/injector -B ${CMAKE_CURRENT_BINARY_DIR}/injector64 -G "${CMAKE_GENERATOR}" -A x64 -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        COMMAND_ERROR_IS_FATAL ANY
    )
else()
    message(FATAL_ERROR "Only Visual Studio generator is supported.")
endif()

add_custom_target(
    build_injector32
    ALL
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/injector32 --config $<CONFIG>
)
add_custom_target(
    build_injector64
    ALL
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/injector64 --config $<CONFIG>
)

install(CODE "
    execute_process(
        COMMAND \"${CMAKE_COMMAND}\" --install \"${CMAKE_CURRENT_BINARY_DIR}/injector32\" --config \"$<CONFIG>\" --prefix \"$<INSTALL_PREFIX>\"
        COMMAND_ERROR_IS_FATAL ANY
    )
")
install(CODE "
    execute_process(
        COMMAND \"${CMAKE_COMMAND}\" --install \"${CMAKE_CURRENT_BINARY_DIR}/injector64\" --config \"$<CONFIG>\" --prefix \"$<INSTALL_PREFIX>\"
        COMMAND_ERROR_IS_FATAL ANY
    )
")

add_subdirectory(main)
