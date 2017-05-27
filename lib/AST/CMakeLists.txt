cmake_minimum_required(VERSION 2.8)

add_library(ast STATIC
  ast.cpp
)

target_link_libraries(ast ${Boost_LIBRARIES})
