file(GLOB MY_SOURCES "./*.cpp")
file(GLOB MY_HEADERS "./*.hpp")
add_library(multicast_channel_lib ${MY_SOURCES} ${MY_HEADERS})
target_include_directories(multicast_channel_lib PUBLIC ${PROJECT_SOURCE_DIR})