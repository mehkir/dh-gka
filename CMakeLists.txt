cmake_minimum_required(VERSION 3.26.4)
project(multicast)

set(CMAKE_CXX_STANDARD 26)

add_executable(sender sender.cpp)
add_executable(receiver receiver.cpp)
# ------------------------------------------------ #
add_subdirectory(Member)
add_executable(start-member start-member.cpp)
target_link_libraries(start-member PUBLIC mclib)
target_include_directories(start-member PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/Member")