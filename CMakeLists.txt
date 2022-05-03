cmake_minimum_required(VERSION 3.20)
project(sik_zad3)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(BOOST_ROOT ~/boost_1_79_0/~)
find_package(Boost 1.79.0 COMPONENTS program_options)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(sik-zad3 main.cpp)
    target_link_libraries(sik-zad3 ${Boost_LIBRARIES})
endif()