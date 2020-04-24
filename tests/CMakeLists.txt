include(GoogleTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(toml_test
    parse_test.cc
)

target_link_libraries(
    toml_test
    toml
    gtest_main
)

gtest_discover_tests(toml_test)