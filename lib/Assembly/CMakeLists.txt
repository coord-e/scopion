cmake_minimum_required(VERSION 2.8)

file(GLOB files *.cpp)

add_library(assembly STATIC ${files})

target_link_libraries(assembly ${LLVM_LIBRARIES} ${Boost_LIBRARIES})
