cmake_minimum_required(VERSION 2.8)

add_library(assembly STATIC
  assembly.cpp
)

target_link_libraries(assembly ${LLVM_LIBRARIES} ${Boost_LIBRARIES})
