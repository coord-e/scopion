cmake_minimum_required(VERSION 2.8)

file(GLOB files *.cpp)

add_library(parser STATIC ${files})

target_link_libraries(parser ${Boost_LIBRARIES})
