cmake_minimum_required(VERSION 2.8)

file(GLOB files *.cpp)

add_library(ast STATIC ${files})

target_link_libraries(ast ${Boost_LIBRARIES})
