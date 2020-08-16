cmake_minimum_required(VERSION 3.14)
project(cpp_websocket)

set(CMAKE_CXX_STANDARD 17)

set(Boost_DIR /home/u/mylib/boost/1.73/lib/cmake/Boost-1.73.0)
find_package(Boost REQUIRED thread)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Threads REQUIRED)

add_executable(cpp_websocket main.cpp)
target_link_libraries(cpp_websocket ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(cpp_websocket ${Boost_LIBRARIES})