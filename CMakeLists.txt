set(CMAKE_C_COMPILER /opt/gcc-11.2/bin/gcc-11.2)
set(CMAKE_CXX_COMPILER /opt/gcc-11.2/bin/g++-11.2)

set(CMAKE_CXX_STANDARD 20)

cmake_minimum_required(VERSION 3.18)
project(sik_zad2)

SET(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/opt/gcc-11.2/lib64")
SET(CMAKE_CXX_FLAGS "-std=gnu++20 -Wall -Wextra -Wconversion -Werror -O2 \
    -pthread")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.74.0 COMPONENTS program_options)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(robots-client client.cpp Client.h ClientSerialization.h
            ByteStream.h ClientState.h Buffer.h common.h   utils.h )
    target_link_libraries(robots-client ${Boost_LIBRARIES})
endif()
