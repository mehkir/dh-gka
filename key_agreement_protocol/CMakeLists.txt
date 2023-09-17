file(GLOB MY_SOURCES "./*.cpp")
file(GLOB MY_HEADERS "./*.hpp")
add_library(key_agreement_protocol_lib ${MY_SOURCES} ${MY_HEADERS})
target_include_directories(key_agreement_protocol_lib PUBLIC ${PROJECT_SOURCE_DIR}/member)