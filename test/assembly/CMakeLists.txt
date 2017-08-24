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

cmake_minimum_required(VERSION 2.8)

add_executable(assembly_test main.cpp)
target_link_libraries(assembly_test scopion gtest gtest_main pthread)

add_test(
    NAME assembly
    COMMAND $<TARGET_FILE:assembly_test>
    )

set_property(
    TEST assembly
    PROPERTY LABELS lib assembly
    )
