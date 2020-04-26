### These flags should be removed eventually
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(parse parse.cpp)
target_link_libraries(parse toml)

add_executable(toml-parser parse_stdin.cpp)
target_link_libraries(toml-parser toml)

add_executable(toml-build build_toml.cpp)
target_link_libraries(toml-build toml)

add_executable(toml-conversions conversions.cpp)
target_link_libraries(toml-conversions toml)