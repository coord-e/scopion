cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0048 NEW)

project(scopion VERSION 0.0.2 LANGUAGES CXX)

set(SCOPION_CACHE_DIR $ENV{HOME}/.scopion)

include(cmake/gitinfo.cmake)

add_subdirectory(include)

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if(NOT ENV{LLVM_DIR})
    set(ENV{LLVM_DIR} /usr/local/Cellar/llvm/4.0.0/)
endif()

if(NOT ENV{Clang_DIR})
    set(ENV{Clang_DIR} /usr/local/Cellar/llvm/4.0.0/)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_FLAGS "-std=c++14 -pedantic-errors -Wall -Werror -Weverything -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-parameter -Wno-global-constructors -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS -DBOOST_MPL_LIMIT_LIST_SIZE=50")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
set(CCMAKE_CXX_FLAGS_MINSIZEREL "-Os -march=native")

option(ENABLE_CLANG_TIDY
  "Enable clang-tidy's diagnosis. This sometimes makes building slow."
  ON)
if(ENABLE_CLANG_TIDY)
  message(STATUS "Enable Clang-Tidy")
set(CMAKE_CXX_CLANG_TIDY
    clang-tidy
      -checks=-*,google-readability-casting
      -fix
      -fix-errors
      -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

include_directories(include)
include_directories(SYSTEM extsrc/include)

find_package(LLVM REQUIRED CONFIG)
find_package(Boost COMPONENTS system filesystem REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CLANG_LIBS clang clangAnalysis clangAST clangBasic clangCodeGen clangDriver clangEdit clangFrontend clangLex clangParse clangRewrite clangSema clangSerialization clangTooling)
link_directories($ENV{LLVM_DIR}/lib)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS}) # LLVM headers have a lot of warnings with -Weverything
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBRARIES all)

include_directories(${Boost_INCLUDE_DIRS})

include(cmake/clang-format.cmake)

if(NOT WITHOUT_TEST)
   enable_testing()
   include(cmake/gtest.cmake)
endif()

add_subdirectory(lib)
add_subdirectory(src)

if(NOT WITHOUT_TEST)
   add_subdirectory(test)
endif()

add_subdirectory(utils)

include(cmake/cpack.cmake)
