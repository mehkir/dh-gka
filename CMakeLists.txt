cmake_minimum_required(VERSION 3.26.4)
project(multicast)

set(CMAKE_CXX_STANDARD 26)

add_executable(sender sender.cpp)
add_executable(receiver receiver.cpp)
add_subdirectory(multicast_channel)
# ------------------------------------------------ #
add_executable(myapp MyApp.cpp)
target_link_libraries(myapp PUBLIC mclib)
target_include_directories(myapp PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/multicast_channel")