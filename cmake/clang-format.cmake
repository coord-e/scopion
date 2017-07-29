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
