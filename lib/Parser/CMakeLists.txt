cmake_minimum_required(VERSION 2.8)

add_library(parser STATIC
  parser.cpp
)

target_link_libraries(parser ${Boost_LIBRARIES})
