set(CMAKE_CXX_STANDARD 20)

cmake_minimum_required(VERSION 3.18)
project(sik_zad3)

SET(CMAKE_CXX_FLAGS "-std=c++20 -pthread")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(BOOST_ROOT ~/boost_1_74_0/boost_root)
find_package(Boost 1.74.0 COMPONENTS program_options)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(sik-zad3-client client.cpp common.h TcpClientConnection.h Messages.h ByteStream.h Randomizer.h ServerState.h Client.h ClientState.h Buffer.h utils.h)
    add_executable(sik-zad3-server server.cpp common.h TcpClientConnection.h Messages.h ByteStream.h server.cpp Randomizer.h ServerState.h Buffer.h Server.h utils.h)
    target_link_libraries(sik-zad3-client ${Boost_LIBRARIES})
    target_link_libraries(sik-zad3-server ${Boost_LIBRARIES})
endif()