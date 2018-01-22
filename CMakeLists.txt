# CMakeLists.txt
#
# (c) copyright 2017 coord.e
#
# This file is part of scopion.
#
# scopion is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# scopion is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with scopion.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0048 NEW)

project(scopion VERSION 0.0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SCOPION_ETC_DIR ${CMAKE_INSTALL_PREFIX}/etc/scopion)
set(SCOPION_CACHE_DIR .scopion)

include(cmake/gitinfo.cmake)

add_subdirectory(include)

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if(NOT ENV{LLVM_DIR})
    set(ENV{LLVM_DIR} /usr/local/opt/llvm/)
endif()

if(NOT CONFIG)
  set(CONFIG RELEASE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-std=c++14 -pedantic-errors -Wall -Werror -Weverything -Wno-float-equal -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-parameter -Wno-global-constructors -Wno-exit-time-destructors -Wno-zero-as-null-pointer-constant)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-std=c++14 -pedantic-errors -Wall -Wextra -Werror -Wno-float-equal -Wno-padded -Wno-unused-parameter -Wno-global-constructors -Wno-exit-time-destructors -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused -Wno-zero-as-null-pointer-constant)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4)
endif()

if(CONFIG STREQUAL "DEBUG")
  add_compile_options(-O0 -g3)
elseif(CONFIG STREQUAL "RELEASE")
  add_compile_options(-O3 -march=native)
endif()

option(ENABLE_CLANG_TIDY
  "Enable clang-tidy's diagnosis. This sometimes makes building slow."
OFF)
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
include_directories(SYSTEM extsrc/args)
include_directories(SYSTEM extsrc/rang/include)

find_package(LLVM REQUIRED CONFIG)
if(${LLVM_PACKAGE_VERSION} VERSION_LESS "5.0")
  message(FATAL_ERROR "Found LLVM ${LLVM_PACKAGE_VERSION}, but 5.0 or newer is required")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.62.0 REQUIRED COMPONENTS filesystem system)

link_directories($ENV{LLVM_DIR}/lib)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS}) # LLVM headers have a lot of warnings with -Weverything
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBRARIES core asmparser irreader native support target passes object interpreter)

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
