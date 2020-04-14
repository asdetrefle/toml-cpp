### These flags should be removed eventually
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(parse parse.cpp)
target_link_libraries(parse tominal)

add_executable(toml-parser parse_stdin.cpp)
target_link_libraries(toml-parser tominal)

if(FALSE)
add_executable(cpptoml-build build_toml.cpp)
target_link_libraries(cpptoml-build cpptoml)
set_target_properties(cpptoml-build PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED YES)

add_executable(cpptoml-conversions conversions.cpp)
target_link_libraries(cpptoml-conversions cpptoml)
set_target_properties(cpptoml-conversions PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED YES)
endif()