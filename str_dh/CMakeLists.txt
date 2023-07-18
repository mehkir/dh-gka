file(GLOB MY_SOURCES "./*.cpp")
file(GLOB MY_HEADERS "./*.hpp")
add_subdirectory(multicast_channel)

add_library(str_dh_lib ${MY_SOURCES} ${MY_HEADERS})