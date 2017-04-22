cmake_minimum_required(VERSION 2.8)

add_library(Parser STATIC
  Parser.cpp
)

target_link_libraries(Parser ${Boost_LIBRARIES})
