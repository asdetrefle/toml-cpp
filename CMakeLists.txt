cmake_minimum_required(VERSION 3.21)
project(toml VERSION 0.2)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

option(TOML_BUILD_EXAMPLES "Build examples" OFF)
option(TOML_BUILD_TESTS "Build tests" ON)

add_library(toml INTERFACE)
target_include_directories(
    toml INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)
target_compile_features(toml INTERFACE cxx_std_17)

if(TOML_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY      https://github.com/google/googletest.git
        GIT_TAG             master
        SOURCE_DIR          "${CMAKE_BINARY_DIR}/googletest-src"
        BINARY_DIR          "${CMAKE_BINARY_DIR}/googletest-build"
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()

if(TOML_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()