include(GoogleTest)

add_executable(toml_test
    toml_test.cc
)

target_link_libraries(
    toml_test
    gtest_main
)

gtest_discover_tests(toml_test)