# clang-format.cmake - configure about clang-format
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

cmake_minimum_required(VERSION 3.3)

option(FORMAT_BEFORE_BUILD
  "If the command clang-format is avilable, format source files before each build.\
Turn this off if the build time is too slow."
  ON)
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
  message(STATUS "Enable Clang-Format")
  file(GLOB_RECURSE files src/*.cpp test/*.cpp lib/*.cpp include/*.hpp)
  add_custom_target(
    format
    COMMAND clang-format -i -style=file ${files}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
  add_custom_target(format_build)
  if(FORMAT_BEFORE_BUILD)
    message(STATUS "Enable build-time formatting")
    add_dependencies(format_build format)
  endif()
else()
    message(WARNING "Could not find clang-format")
endif()
