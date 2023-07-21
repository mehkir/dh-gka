file(GLOB MY_SOURCES "./*.cpp")
file(GLOB MY_HEADERS "./*.hpp")
add_library(mclib ${MY_SOURCES} ${MY_HEADERS})
target_include_directories(mclib PUBLIC ${PROJECT_SOURCE_DIR})