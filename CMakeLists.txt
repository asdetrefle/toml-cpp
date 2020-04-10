cmake_minimum_required(VERSION 3.14.0)
project(toml VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

option(TOML_BUILD_EXAMPLE "Build examples" OFF)
option(TOML_BUILD_TEST "Build tests" ON)

# Import tomlplusplus interface library
add_library(toml INTERFACE IMPORTED)
set_target_properties(
    toml PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}"
)

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
    add_subdirectory(test)
endif()