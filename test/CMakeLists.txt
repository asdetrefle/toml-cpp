include(GoogleTest)

add_executable(toml_test
    parse_test.cc
)

target_link_libraries(
    toml_test
    toml
    gtest_main
)

gtest_discover_tests(toml_test)