cmake_minimum_required(VERSION 3.11)
project(topxgboost LANGUAGES CXX C)

message(STATUS "CMake version ${CMAKE_VERSION}")
if (MSVC)
    cmake_minimum_required(VERSION 3.11)
endif (MSVC)

add_subdirectory(${PROJECT_SOURCE_DIR}/host)
add_subdirectory(${PROJECT_SOURCE_DIR}/enclave)
