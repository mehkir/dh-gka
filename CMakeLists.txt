cmake_minimum_required(VERSION 3.26.4)
project(multicast)

set(CMAKE_CXX_STANDARD 26)

add_compile_definitions(ECC_DH PROTO_STR_DH)

add_subdirectory(multicast_channel)
add_subdirectory(message_handler)
add_subdirectory(str_dh)
add_subdirectory(distributed_dh)
add_subdirectory(statistics)

add_executable(sender sender.cpp)
add_executable(receiver receiver.cpp)
# ------------------------------------------------ #
add_executable(multicast-app-example multicast-app-example.cpp)
target_link_libraries(multicast-app-example PUBLIC multicast_channel_lib)
target_include_directories(multicast-app-example PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/multicast_channel)
# ------------------------------------------------ #
add_executable(multicast-dh-example multicast-dh-example.cpp)
target_include_directories(multicast-dh-example PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/str_dh ${PROJECT_SOURCE_DIR}/distributed_dh)
target_link_libraries(multicast-dh-example PUBLIC str_dh_lib distributed_dh_lib)
# ------------------------------------------------ #
add_executable(statistics-writer-main statistics-writer-main.cpp)
target_include_directories(statistics-writer-main PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/statistics)
target_link_libraries(statistics-writer-main PUBLIC statistics_lib)
# ------------------------------------------------ #
add_executable(statistics-recorder-main statistics-recorder-main.cpp)
target_include_directories(statistics-recorder-main PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/statistics)
target_link_libraries(statistics-recorder-main PUBLIC statistics_lib)
# ------------------------------------------------ #
add_executable(multicast-app-testframe multicast-app-testframe.cpp)
target_include_directories(multicast-app-testframe PUBLIC ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/multicast_channel ${PROJECT_SOURCE_DIR}/key_agreement_protocol ${PROJECT_SOURCE_DIR}/message_handler ${PROJECT_SOURCE_DIR}/type_definitions)
target_link_libraries(multicast-app-testframe PUBLIC multicast_channel_lib message_handler_lib cryptopp crypto boost_serialization boost_system)
# ------------------------------------------------ #
add_executable(testframe testframe.cpp)