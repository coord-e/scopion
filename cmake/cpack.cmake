# cpack.cmake - configure about CPack
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

cmake_minimum_required(VERSION 2.6)

set(CPACK_PACKAGE_NAME "scopion")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "scopion programming language")
set(CPACK_PACKAGE_DESCRIPTION "scopion is a new programming language with simple syntax.")
set(CPACK_PACKAGE_VENDOR "coord.e")
set(CPACK_PACKAGE_CONTACT "me@coord-e.com")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}_${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}")

if(EXISTS /etc/debian_version)
set(CPACK_GENERATOR "DEB;TBZ2;TGZ;ZIP")
else(EXISTS /etc/debian_version)
set(CPACK_GENERATOR "TBZ2;TGZ;ZIP")
endif(EXISTS /etc/debian_version)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "clang-5.0, llvm-5.0, libgc-dev, exuberant-ctags")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})

include(CPack)
