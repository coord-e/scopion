cmake_minimum_required(VERSION 2.6)

configure_file(scopion-h2ir.in ${CMAKE_CURRENT_BINARY_DIR}/scopion-h2ir)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/scopion-h2ir DESTINATION bin)
