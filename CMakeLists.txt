cmake_minimum_required(VERSION 3.14.0)
project(toml VERSION 0.1)

option(TOML_BUILD_EXAMPLE "Build examples" OFF)
option(TOML_BUILD_TEST "Build tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

# Import tomlplusplus interface library
add_library(toml INTERFACE IMPORTED)

set_target_properties(
    toml PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}"
    CXX_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(TOML_BUILD_TEST)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY      https://github.com/google/googletest.git
        GIT_TAG             master
        SOURCE_DIR          "${CMAKE_BINARY_DIR}/googletest-src"
        BINARY_DIR          "${CMAKE_BINARY_DIR}/googletest-build"
    )

    enable_testing()
    add_subdirectory(test)
endif()

