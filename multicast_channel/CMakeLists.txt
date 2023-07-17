file(GLOB MY_SOURCES "./*.cpp")
file(GLOB MY_HEADERS "./*.hpp")
add_library(mclib ${MY_SOURCES} ${MY_HEADERS})