cmake_minimum_required(VERSION 3.8)

project(scopion CXX)

set(CMAKE_MACOSX_RPATH 1)

if(NOT ENV{LLVM_DIR})
    set(ENV{LLVM_DIR} /usr/local/Cellar/llvm/4.0.0/)
endif(NOT ENV{LLVM_DIR})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "-std=c++14 -pedantic-errors -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
set(CCMAKE_CXX_FLAGS_MINSIZEREL "-Os -march=native")

include_directories(include)
include_directories(extsrc/include)

find_package(LLVM REQUIRED CONFIG)
find_package(Boost COMPONENTS system filesystem REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBRARIES all)

include_directories(${Boost_INCLUDE_DIRS})

if(NOT WITHOUT_TEST)
   enable_testing()
   include(cmake/gtest.cmake)
endif()

add_subdirectory(lib)
add_subdirectory(src)

if(NOT WITHOUT_TEST)
   add_subdirectory(test)
endif()
