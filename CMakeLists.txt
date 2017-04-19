cmake_minimum_required(VERSION 2.8)

project(scopion)

set(ENV{LLVM_DIR} /usr/local/Cellar/llvm/4.0.0/)

add_definitions(-std=c++14)
include_directories(.)

find_package(LLVM REQUIRED CONFIG)
find_package(Boost REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBRARIES support core irreader)

include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(src/scopion)

#if(NOT without-test)
#    enable_testing()
#    include(cmake/gtest.cmake)
#    add_subdirectory(test)
#endif()
