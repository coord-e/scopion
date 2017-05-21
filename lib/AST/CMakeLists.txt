cmake_minimum_required(VERSION 2.8)

add_library(AST STATIC
  AST.cpp
)

target_link_libraries(AST ${Boost_LIBRARIES})
