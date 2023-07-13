cmake_minimum_required(VERSION 3.26.4)
project(multicast)

set(CMAKE_CXX_STANDARD 26)

add_executable(sender sender.cpp)
add_executable(receiver receiver.cpp)