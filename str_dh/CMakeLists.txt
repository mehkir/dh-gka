file(GLOB MY_SOURCES "./*.cpp")
file(GLOB MY_HEADERS "./*.hpp")
add_library(str_dh_lib ${MY_SOURCES} ${MY_HEADERS})
target_include_directories(str_dh_lib PUBLIC ${PROJECT_SOURCE_DIR}/multicast_channel)