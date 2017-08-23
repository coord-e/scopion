cmake_minimum_required(VERSION 3.8)

configure_file(config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/scopion/config.hpp)

install(DIRECTORY . DESTINATION include
FILES_MATCHING PATTERN "*.hpp")
