cmake_minimum_required(VERSION 3.14.0)
project(tominal VERSION 0.1)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

option(TOML_BUILD_EXAMPLE "Build examples" OFF)
option(TOML_BUILD_TEST "Build tests" ON)

add_library(tominal INTERFACE IMPORTED)
set_target_properties(
    tominal PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/include"
)
target_compile_features(tominal INTERFACE cxx_std_17)

if(TOML_BUILD_TEST)
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

if(TOML_BUILD_EXAMPLE)
    add_subdirectory(examples)
endif()